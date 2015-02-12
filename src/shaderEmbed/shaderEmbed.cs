using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections;
using System.IO;
using System.Reflection;
using System.Resources;
using System.Text.RegularExpressions;

using Microsoft.Build.Framework;
using Microsoft.Build.CPPTasks;
using Microsoft.Build.Utilities;

namespace shaderEmbed.Build.CPPTasks
{
    public class ShaderEmbed : TrackedVCToolTask
    {
        private ArrayList switchOrderList;

        protected override string ToolName
        {
            get
            {
                return "ShaderToVar.exe";
            }
        }

        protected override string GenerateFullPathToTool()
        {
            return ToolPath+"\\"+this.ToolName;
        }

        protected override ArrayList SwitchOrderList
        {
            get
            {
                return this.switchOrderList;
            }
        }

//        [Required]
//        public string ToolPath
//        { get; set; }

        [Required]
        public string EmbedFileName
        { get; set; }

        [Required]
        public int Embed
        { get; set; }

        public string PrecompiledHeaderFile
        { get; set; }
        
        [Required]
        public virtual ITaskItem[] Sources
        {
            get
            {
                if (base.IsPropertySet("Sources"))
                {
                    return base.ActiveToolSwitches["Sources"].TaskItemArray;
                }
                return null;
            }
            set
            {
                base.ActiveToolSwitches.Remove("Sources");
                ToolSwitch toolSwitch = new ToolSwitch(ToolSwitchType.ITaskItemArray);
                toolSwitch.Separator = " ";
                toolSwitch.Required = true;
                toolSwitch.ArgumentRelationList = new ArrayList();
                toolSwitch.TaskItemArray = value;
                base.ActiveToolSwitches.Add("Sources", toolSwitch);
                base.AddActiveSwitchToolValue(toolSwitch);
            }
        }

        public virtual bool BuildingInIde
        {
            get
            {
                return base.IsPropertySet("BuildingInIde") && base.ActiveToolSwitches["BuildingInIde"].BooleanValue;
            }
            set
            {
                base.ActiveToolSwitches.Remove("BuildingInIde");
                ToolSwitch toolSwitch = new ToolSwitch(ToolSwitchType.Boolean);
                toolSwitch.ArgumentRelationList = new ArrayList();
                toolSwitch.Name = "BuildingInIde";
                toolSwitch.BooleanValue = value;
                base.ActiveToolSwitches.Add("BuildingInIde", toolSwitch);
                base.AddActiveSwitchToolValue(toolSwitch);
            }
        }

        public virtual string TrackerLogDirectory
        {
            get
            {
                if (base.IsPropertySet("TrackerLogDirectory"))
                {
                    return base.ActiveToolSwitches["TrackerLogDirectory"].Value;
                }
                return null;
            }
            set
            {
                base.ActiveToolSwitches.Remove("TrackerLogDirectory");
                ToolSwitch toolSwitch = new ToolSwitch(ToolSwitchType.Directory);
                toolSwitch.DisplayName = "Tracker Log Directory";
                toolSwitch.Description = "Tracker Log Directory.";
                toolSwitch.ArgumentRelationList = new ArrayList();
                toolSwitch.Value = VCToolTask.EnsureTrailingSlash(value);
                base.ActiveToolSwitches.Add("TrackerLogDirectory", toolSwitch);
                base.AddActiveSwitchToolValue(toolSwitch);
            }
        }

        protected override bool UseUnicodeOutput
        {
            get
            {
                return this.BuildingInIde;
            }
        }

        protected override string[] ReadTLogNames
        {
            get
            {
                return new string[]
				{
					"shaderEmbed.read.1.tlog",
					"shaderEmbed.*.read.1.tlog"
				};
            }
        }
        protected override string[] WriteTLogNames
        {
            get
            {
                return new string[]
				{
					"shaderEmbed.write.1.tlog",
					"shaderEmbed.*.write.1.tlog"
				};
            }
        }
        protected override string CommandTLogName
        {
            get
            {
                return "shaderEmbed.command.1.tlog";
            }
        }
        protected override string TrackerIntermediateDirectory
        {
            get
            {
                if (this.TrackerLogDirectory != null)
                {
                    return this.TrackerLogDirectory;
                }
                return string.Empty;
            }
        }
        protected override ITaskItem[] TrackedInputFiles
        {
            get
            {
                return this.Sources;
            }
        }

        public ShaderEmbed()
            : base(new ResourceManager("shaderEmbed.Build.CppTasks.Properties.Resources", Assembly.GetExecutingAssembly()))
        {
            this.switchOrderList = new ArrayList();
            this.switchOrderList.Add("Embed");
            this.switchOrderList.Add("Sources");
            this.switchOrderList.Add("BuildingInIde");
        }

        protected override void ValidateRelations()
        {
        }

        protected override ITaskItem[] AssignOutOfDateSources(ITaskItem[] sources)
        {
            base.ActiveToolSwitches["Sources"].TaskItemArray = sources;
            return sources;
        }

        protected override string GenerateCommandLineCommands()
        {
            string embedValue;

            switch(Embed)
            {
                case 0:
                default:
                    embedValue="copy";
                    break;
                case 1:
                    embedValue="removeComments";
                    break;
                case 2:
                    embedValue="minify";
                    break;
                case 3:
                    embedValue="optimize";
                    break;
            }
            string commandLine;
                
            if(PrecompiledHeaderFile != "")
                commandLine= "-a "+PrecompiledHeaderFile+" -c " + embedValue + " -i " + Sources[0] + " -o " + EmbedFileName;
            else
                commandLine= "-c " + embedValue + " -i " + Sources[0] + " -o " + EmbedFileName;

            return commandLine;
        }

        protected override string GenerateResponseFileCommands()
        {
            return string.Empty;
        }
    };
}
