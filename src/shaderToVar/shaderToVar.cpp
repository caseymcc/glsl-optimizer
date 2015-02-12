// shaderToVar.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <vector>

#include "..\getopt\getopt.h"
#include "..\glsl\glsl_optimizer.h"
#include "..\mesa\main\glheader.h"
#include "..\mesa\main\mtypes.h"

enum ConversionType
{
	copy,
	removeComments,
	minify,
	optimize
};

std::vector<std::string> g_includeFiles;

void convertFile(ConversionType type, std::string inputFile, std::string outputFile);
void copyToVar(std::string inputFile, std::string outputFile, bool removeComments=false);
void minifyShader(std::string inputFile, std::string outputFile, bool optimize, glslopt_target target, glslopt_shader_type type);

int main(int argc, char *argv[])
{
	int c;
	int errors=0;
	char *inputFile=nullptr;
	char *outputFile=nullptr;
	ConversionType conversionType=copy;

	while((c=getopt(argc, argv, "c:i:o:a:"))!=-1)
	{
		switch(c)
		{
		case 'i':
			inputFile=optarg;
			break;
		case 'o':
			outputFile=optarg;
			break;
		case 'c':
			if(strcmp(optarg, "copy")==0)
				conversionType=copy;
			else if(strcmp(optarg, "removeComments")==0)
				conversionType=removeComments;
			else if(strcmp(optarg, "minify")==0)
				conversionType=minify;
			else if(strcmp(optarg, "optimize")==0)
				conversionType=optimize;
			else
			{ 
				printf("Option -c requires conversion type [copy, removeComments, minify, optimize]\n");
				errors++;
			}
			break;
		case 'a':
			g_includeFiles.push_back(optarg);
			break;
		case ':':
			switch(optopt)
			{
			case 'c':
				printf("Option -c requires conversion type [copy, removeComments, minify, optimize]\n");
				break;
			case 'i':
				printf("Option -i requires a file\n");
				break;
			case 'o':
				printf("Option -o requires a file\n");
				break;
			case 'a':
				printf("Option -p requires then name of file to add as #include \"filename\"\n");
				break;
			}
			errors++;
			break;
		case '?':
			printf("Unrecognized option: '-%c'\n", optopt);
			errors++;
			break;
		}
	}

	if(inputFile==nullptr)
		printf("No input file provided use -i [file]");
	
	if(outputFile==nullptr)
		printf("No output file provided use -o [file]");

	convertFile(conversionType, inputFile, outputFile);

	if(errors>0)
		return 0;

	return 0;
}

void convertFile(ConversionType type, std::string inputFile, std::string outputFile)
{
	switch(type)
	{
	case copy:
		copyToVar(inputFile, outputFile, false);
		break;
	case removeComments:
		copyToVar(inputFile, outputFile, true);
		break;
	case minify:
		minifyShader(inputFile, outputFile, false, kGlslTargetOpenGL, kGlslOptShaderFragment);
		break;
	case optimize:
		minifyShader(inputFile, outputFile, true, kGlslTargetOpenGL, kGlslOptShaderFragment);
		break;
	}
}

void copyToVar(std::string inputFile, std::string outputFile, bool removeComments)
{
	FILE *input=fopen(inputFile.c_str(), "r");

	if(input==nullptr)
	{
		printf("Could not open input file %s", inputFile.c_str());
		return;
	}

	FILE *output=fopen(outputFile.c_str(), "w");

	if(output==nullptr)
	{
		printf("Could not open output file %s", outputFile.c_str());
		fclose(input);
		return;
	}

	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fileName[_MAX_FNAME];
	char ext[_MAX_EXT];

	_splitpath(inputFile.c_str(), drive, dir, fileName, ext);

	if(!g_includeFiles.empty())
	{
		for(std::string &includeFile:g_includeFiles)
		{
			fprintf(output, "#include \"%s\"\n", includeFile.c_str());
		}
	}
	fprintf(output, "#include <string>\n\n");
	fprintf(output, "std::string %s_%s(\n", fileName, &ext[1]);

	char buffer[4096];
	bool spanComment=false;

	while(fgets(buffer, 4096, input)!=nullptr)
	{
		size_t length=strlen(buffer);

		while((buffer[length-1]=='\n')||(buffer[length-1]=='\r'))
		{
			buffer[length-1]=0;
			length--;
		}

		if(removeComments)
		{
			char *pos=nullptr;
			
			if(!spanComment)
			{
				pos=strstr(buffer, "/*");

				if(pos!=nullptr)
					spanComment=true;
			}

			if(spanComment)
			{
				char *spanPos;
				
				if(pos==nullptr)
					pos=buffer;
				spanPos=strstr(pos, "*/");

				if(spanPos!=nullptr)
				{
					char *endPos=buffer+strlen(buffer);

					spanPos+=2;
					while(spanPos!=endPos)
					{
						*pos=*spanPos;
						pos++;
						spanPos++;
					}
					*pos=0;
					spanComment=false;
				}
				else
					*pos=0;
			}
				
			pos=strstr(buffer, "//");

			if(pos!=nullptr)
				*pos=0;

			if(strlen(buffer)<=0)
				continue;
		}

		fprintf(output, "\"");
		fputs(buffer, output);
		fprintf(output, "\"\n");
	}
	fprintf(output, ");\n");

	fclose(input);
	fclose(output);
}

void minifyShader(std::string inputFile, std::string outputFile, bool optimize, glslopt_target target, glslopt_shader_type type)
{
	FILE *input=fopen(inputFile.c_str(), "r");

	if(input==nullptr)
	{
		printf("Could not open input file %s", inputFile.c_str());
		return;
	}

	FILE *output=fopen(outputFile.c_str(), "w");

	if(output==nullptr)
	{
		printf("Could not open output file %s", outputFile.c_str());
		fclose(input);
		return;
	}

	fseek(input, 0, SEEK_END);
	long fileSize=ftell(input);
	fseek(input, 0, SEEK_SET);

	char *shaderSource=(char *)malloc(fileSize+1);

	fread(shaderSource, sizeof(char), fileSize, input);

	glslopt_ctx *glslOptContext=glslopt_initialize(target);
	glslopt_shader *shader=glslopt_optimize(glslOptContext, type, shaderSource, kGlslOptionMinimize);

	if(glslopt_get_status(shader))
	{
		const char *outputSource;

		if(optimize)
			outputSource=glslopt_get_output(shader);
		else
			outputSource=glslopt_get_raw_output(shader);

		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fileName[_MAX_FNAME];
		char ext[_MAX_EXT];

		_splitpath(inputFile.c_str(), drive, dir, fileName, ext);

		if(!g_includeFiles.empty())
		{
			for(std::string &includeFile:g_includeFiles)
			{
				fprintf(output, "#include \"%s\"\n", includeFile.c_str());
			}
		}
		fprintf(output, "#include <string>\n");
		fprintf(output, "std::string %s_%s(\"", fileName, &ext[1]);


		std::string outputString(outputSource);
		size_t pos=0;
		
		for(std::string::iterator pos=outputString.begin(); pos<outputString.end();)
		{
			if(*pos=='\n')
			{
				*pos='n';
				pos=outputString.insert(pos, '\\');
				pos+=2;
			}
			else
				pos++;
		}
//		while((pos=outputString.find('\n', pos))!=std::string::npos)
//		{
//			outputString.insert(outputString.begin()+pos+1, '\\');
//			pos++;
//		}
		fwrite(outputString.c_str(), sizeof(char), outputString.size(), output);
		fprintf(output, "\");");

	}

	glslopt_cleanup(glslOptContext);

	fclose(input);
	fclose(output);
}