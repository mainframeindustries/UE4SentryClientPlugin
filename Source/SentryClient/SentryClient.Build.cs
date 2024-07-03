// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;  // for path tools
using System.Linq;

public class SentryClient : ModuleRules
{
	public SentryClient(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
#if UE_5_2_OR_LATER
        IWYUSupport = IWYUSupport.Full;
#else
		bEnforceIWYU = true;
#endif

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

		// we support either Crashpad or breakpad.  Crashpad is the default and uploads immediately,
		// but for windows, it can be problematic if the game is running under anti-cheat software.
		// In that case, breakpad can be used instead, which will upload crashes upon the next run
		bool bUseCrashPad = true;
		
		bool bIsWindows = Target.Platform == UnrealTargetPlatform.Win64;
		if (bIsWindows)
		{
			// if you want to use breakpad instead on windows, the libraries must be rebuild
			// and the following inverted.
			bUseCrashPad = true;
		}

		if (!SentryDisable && bIsWindows)
		{
			SentryHavePlatform = true;
			SentryLibs = new string[]
			{
				"sentry.lib",
			};
			if (bUseCrashPad)
			{
				SentryPlatform = Path.Combine(SentryRoot, "Win64-Crashpad");
				SentryLibs = SentryLibs.Concat(
					new string[]
					{
						"crashpad_client.lib",
						"crashpad_compat.lib",
						"crashpad_handler_lib.lib",
						"crashpad_minidump.lib",
						"crashpad_snapshot.lib",
						"crashpad_tools.lib",
						"crashpad_util.lib",
						"crashpad_zlib.lib",
						"mini_chromium.lib",
					}
				).ToArray();
				RuntimeDependencies.Add(Path.Combine(SentryPlatform, "bin", "crashpad_handler.exe"));
				RuntimeDependencies.Add(Path.Combine(SentryPlatform, "bin", "crashpad_wer.dll"));
			}
			else
			{
				SentryPlatform = Path.Combine(SentryRoot, "Win64-Breakpad");
				SentryLibs = SentryLibs.Concat(
					new string[]
					{
						"breakpad_client.lib"
					}
				).ToArray() ;
			}
			PublicSystemLibraries.Add("version.lib");
			PublicSystemLibraries.Add("dbghelp.lib");
		}
		else if (!SentryDisable && Target.Platform == UnrealTargetPlatform.Linux)
		{
			SentryHavePlatform = true;
			SentryPlatform = Path.Combine(SentryRoot, "Linux");
			SentryLibs = new string[]
			{
				"libsentry.a",
			};

			if (bUseCrashPad)
			{
				SentryLibs = SentryLibs.Concat(
					new string[]
					{
						"libcrashpad_client.a",
						"libcrashpad_compat.a",
						"libcrashpad_handler_lib.a",
						"libcrashpad_minidump.a",
						"libcrashpad_snapshot.a",
						"libcrashpad_tools.a",
						"libcrashpad_util.a",
						"libmini_chromium.a",
					}
				).ToArray();
				RuntimeDependencies.Add(Path.Combine(SentryPlatform, "bin", "crashpad_handler"));
			}
			else
			{
				SentryLibs = SentryLibs.Concat(
					new string[]
					{
						"libbreakpad_client.a"
					}
				).ToArray();
			}
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
