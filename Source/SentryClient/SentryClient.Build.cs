// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;  // for path tools

public class SentryClient : ModuleRules
{
	public SentryClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;
		
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

		// The root part of the installed sdk (includes/binaries)
		string SentryRoot = Path.Combine(PluginDirectory, "Binaries/ThirdParty/sentry-native");
		string SentryPlatform = "";
		string[] SentryLibs = { };
		bool SentryHavePlatform = false;
		bool SentryDisable = false;  // for internal testing

		if (!SentryDisable && Target.Platform == UnrealTargetPlatform.Win64)
		{
			SentryHavePlatform = true;
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
			PublicSystemLibraries.Add("version.lib");
			PublicSystemLibraries.Add("dbghelp.lib");
			RuntimeDependencies.Add(Path.Combine(SentryPlatform, "bin", "crashpad_handler.exe"));
		}
		else if (!SentryDisable && Target.Platform == UnrealTargetPlatform.Linux)
		{
			SentryHavePlatform = true;
			SentryPlatform = Path.Combine(SentryRoot, "Linux");
			SentryLibs =
				new string[]
				{
					"libsentry.a",
					"libcrashpad_client.a",
					"libcrashpad_compat.a",
					"libcrashpad_handler_lib.a",
					"libcrashpad_minidump.a",
					"libcrashpad_snapshot.a",
					"libcrashpad_tools.a",
					"libcrashpad_util.a",
					"libmini_chromium.a",
				};
			RuntimeDependencies.Add(Path.Combine(SentryPlatform, "bin", "crashpad_handler"));
		}
		else
		{
			; // not supported
		}

		if (SentryHavePlatform)
		{ 
			// anyone including this library can use the sentry api
			PublicIncludePaths.Add(Path.Combine(SentryPlatform, "include"));
			// sentry header file needs thef following since we use static libs
			PublicDefinitions.Add("SENTRY_BUILD_STATIC=1");

			foreach (string lib in SentryLibs) {
				PublicAdditionalLibraries.Add(Path.Combine(SentryPlatform, "lib", lib));
			}
		}
	}
}
