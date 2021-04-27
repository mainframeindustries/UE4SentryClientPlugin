// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;  // for path tools

public class SentryClient : ModuleRules
{
	public SentryClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				//"Slate",
				//"SlateCore",
				"Projects",
				// ... add private dependencies that you statically link with here ...	
				"HTTP",
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);

		// The include path
		string SentryRoot = Path.Combine(ModuleDirectory, "../ThirdParty/sentry-native");
		string SentryPlatform="";
		string[] SentryLibs = { };
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			SentryPlatform = Path.Combine(SentryRoot, "Win64");
			SentryLibs = 
				new string[]
				{
					"sentry.lib",
					"crashpad_client.lib",
					"crashpad_compat.lib",
					"crashpad_handler_lib.lib",
					"crashpad_minidump.lib",
					"crashpad_snapshot.lib",
					"crashpad_tools.lib",
					"crashpad_util.lib",
					"crashpad_zlib.lib",
					"mini_chromium.lib",
				};
			PublicSystemLibraries.Add("winhttp.lib"); // will go away
			PublicSystemLibraries.Add("version.lib");
			PublicSystemLibraries.Add("dbghelp.lib");
			RuntimeDependencies.Add(Path.Combine(SentryPlatform, "bin", "crashpad_handler.exe"));
		}
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			SentryPlatform = Path.Combine(SentryRoot, "Linux");
			//ryLib = "libsentry.a";
		}
		else
        {
			; // not supported
        }

		// anyone including this library can use the sentry api
		PublicIncludePaths.Add(Path.Combine(SentryPlatform, "include"));
		foreach (string lib in SentryLibs) {
			PublicAdditionalLibraries.Add(Path.Combine(SentryPlatform, "lib", lib));
		}
	}
}
