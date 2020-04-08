﻿/*
 * Copyright (C) 2014 Patrick Mours. All rights reserved.
 * License: https://github.com/crosire/reshade#license
 */

using Microsoft.Win32;
using ReShade.Setup.Utilities;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Security.Principal;
using System.Windows;
using System.Windows.Input;

namespace ReShade.Setup
{
	public partial class MainWindow
	{
		bool is64Bit = false;
		bool isHeadless = false;
		bool isElevated = WindowsIdentity.GetCurrent().Owner.IsWellKnown(WellKnownSidType.BuiltinAdministratorsSid);
		bool isFinished = false;
		string configPath = null;
		string targetPath = null;
		string targetName = null;
		string modulePath = null;
		string commonPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData), "ReShade");

		ZipArchive zip;
		IniFile packagesIni;
		IniFile compatibilityIni;

		public MainWindow()
		{
			InitializeComponent();

			var assembly = Assembly.GetExecutingAssembly();
			Title = "ReShade Setup v" + assembly.GetName().Version.ToString(3);

			try
			{
				// Extract archive attached to this executable
				var output = new FileStream(Path.GetTempFileName(), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.ReadWrite | FileShare.Delete, 4096, FileOptions.DeleteOnClose);

				using (var input = File.OpenRead(assembly.Location))
				{
					byte[] block = new byte[512];
					byte[] signature = { 0x50, 0x4B, 0x03, 0x04 }; // PK..

					// Look for archive at the end of this executable and copy it to a file
					while (input.Read(block, 0, block.Length) >= signature.Length)
					{
						if (block.Take(signature.Length).SequenceEqual(signature))
						{
							output.Write(block, 0, block.Length);
							input.CopyTo(output);
							break;
						}
					}
				}

				zip = new ZipArchive(output, ZipArchiveMode.Read, false);
				packagesIni = new IniFile(assembly.GetManifestResourceStream("ReShade.Setup.EffectPackages.ini"));
				compatibilityIni = new IniFile(assembly.GetManifestResourceStream("ReShade.Setup.Compatibility.ini"));
			}
			catch
			{
				MessageBox.Show("This setup archive is corrupted! Please download from https://reshade.me again.");
				Environment.Exit(1);
				return;
			}

			ApiVulkanGlobal.IsChecked = IsVulkanLayerEnabled(Registry.LocalMachine);

			if (isElevated)
			{
				ApiGroup.Visibility = Visibility.Visible;
				ApiVulkanGlobalButton.Visibility = Visibility.Collapsed;
			}
		}

		static void MoveFiles(string sourcePath, string targetPath)
		{
			if (Directory.Exists(targetPath) == false)
			{
				Directory.CreateDirectory(targetPath);
			}

			foreach (string source in Directory.GetFiles(sourcePath))
			{
				string target = targetPath + source.Replace(sourcePath, string.Empty);

				File.Copy(source, target, true);
			}

			// Copy files recursively
			foreach (string source in Directory.GetDirectories(sourcePath))
			{
				string target = targetPath + source.Replace(sourcePath, string.Empty);

				MoveFiles(source, target);
			}
		}
		static bool IsWritable(string targetPath)
		{
			try
			{
				File.Create(Path.Combine(targetPath, Path.GetRandomFileName()), 1, FileOptions.DeleteOnClose);
				return true;
			}
			catch
			{
				return false;
			}
		}

		void AddSearchPath(List<string> searchPaths, string newPath)
		{
			string basePath = Path.GetDirectoryName(ApiVulkan.IsChecked.Value ? commonPath : targetPath);
			Directory.SetCurrentDirectory(basePath);

			bool pathExists = false;

			foreach (var searchPath in searchPaths)
			{
				if (Path.GetFullPath(searchPath) == Path.GetFullPath(newPath))
				{
					pathExists = true;
					break;
				}
			}

			if (!pathExists)
			{
				searchPaths.Add(newPath);
			}
		}
		void WriteSearchPaths(string targetPathShaders, string targetPathTextures)
		{
			// Vulkan uses a common ReShade DLL for all applications, which is not in the location the shaders and textures are installed to, so make paths absolute
			if (ApiVulkan.IsChecked.Value)
			{
				string targetDir = Path.GetDirectoryName(targetPath);
				targetPathShaders = Path.GetFullPath(Path.Combine(targetDir, targetPathShaders));
				targetPathTextures = Path.GetFullPath(Path.Combine(targetDir, targetPathTextures));
			}

			var iniFile = new IniFile(configPath);
			List<string> paths = null;

			iniFile.GetValue("GENERAL", "EffectSearchPaths", out var effectSearchPaths);

			paths = new List<string>(effectSearchPaths ?? new string[0]);
			paths.RemoveAll(string.IsNullOrWhiteSpace);
			{
				AddSearchPath(paths, targetPathShaders);
				iniFile.SetValue("GENERAL", "EffectSearchPaths", paths.ToArray());
			}

			iniFile.GetValue("GENERAL", "TextureSearchPaths", out var textureSearchPaths);

			paths = new List<string>(textureSearchPaths ?? new string[0]);
			paths.RemoveAll(string.IsNullOrWhiteSpace);
			{
				AddSearchPath(paths, targetPathTextures);
				iniFile.SetValue("GENERAL", "TextureSearchPaths", paths.ToArray());
			}

			iniFile.SaveFile();
		}

		bool EnableVulkanLayer(RegistryKey hive)
		{
			try
			{
				if (Directory.Exists(commonPath))
				{
					Directory.Delete(commonPath, true);
				}

				Directory.CreateDirectory(commonPath);
				zip.ExtractToDirectory(commonPath);

				if (Environment.Is64BitOperatingSystem)
				{
					using (RegistryKey key = hive.CreateSubKey(@"Software\Khronos\Vulkan\ImplicitLayers"))
					{
						key.SetValue(Path.Combine(commonPath, "ReShade64.json"), 0, RegistryValueKind.DWord);
					}
				}

				using (RegistryKey key = hive.CreateSubKey(Environment.Is64BitOperatingSystem ? @"Software\Wow6432Node\Khronos\Vulkan\ImplicitLayers" : @"Software\Khronos\Vulkan\ImplicitLayers"))
				{
					key.SetValue(Path.Combine(commonPath, "ReShade32.json"), 0, RegistryValueKind.DWord);
				}

				return true;
			}
			catch
			{
				return false;
			}
		}
		bool DisableVulkanLayer(RegistryKey hive)
		{
			try
			{
				if (Directory.Exists(commonPath))
				{
					Directory.Delete(commonPath, true);
				}

				if (Environment.Is64BitOperatingSystem)
				{
					using (RegistryKey key = hive.CreateSubKey(@"Software\Khronos\Vulkan\ImplicitLayers"))
					{
						key.DeleteValue(Path.Combine(commonPath, "ReShade64.json"));
					}
				}

				using (RegistryKey key = hive.CreateSubKey(Environment.Is64BitOperatingSystem ? @"Software\Wow6432Node\Khronos\Vulkan\ImplicitLayers" : @"Software\Khronos\Vulkan\ImplicitLayers"))
				{
					key.DeleteValue(Path.Combine(commonPath, "ReShade32.json"));
				}

				return true;
			}
			catch
			{
				return false;
			}
		}
		bool IsVulkanLayerEnabled(RegistryKey hive)
		{
			using (RegistryKey key = hive.OpenSubKey(@"Software\Khronos\Vulkan\ImplicitLayers"))
			{
				return key?.GetValue(Path.Combine(commonPath, Environment.Is64BitOperatingSystem ? "ReShade64.json" : "ReShade32.json")) != null;
			}
		}

		void UpdateStatus(string title, string message, string description = null)
		{
			Title = title;
			Message.Text = message ?? string.Empty;
			MessageDescription.Visibility = string.IsNullOrEmpty(description) ? Visibility.Collapsed : Visibility.Visible;
			MessageDescription.Text = description;

			AeroGlass.HideSystemMenu(this);
		}
		void UpdateStatusAndFinish(bool success, string message, string description = null)
		{
			isFinished = true;
			SetupButton.IsEnabled = false; // Use button as text box only

			UpdateStatus(success ? "ReShade Setup was successful!" : "ReShade Setup was not successful!", message, description);

			AeroGlass.HideSystemMenu(this, false);

			if (isHeadless)
			{
				Environment.Exit(success ? 0 : 1);
			}
		}

		bool RestartWithElevatedPrivileges()
		{
			var startInfo = new ProcessStartInfo {
				Verb = "runas",
				FileName = Assembly.GetExecutingAssembly().Location,
				Arguments = $"\"{targetPath}\" --elevated --left {Left} --top {Top}"
			};

			if (ApiD3D9.IsChecked.Value)
				startInfo.Arguments += " --api d3d9";
			if (ApiDXGI.IsChecked.Value)
				startInfo.Arguments += " --api dxgi";
			if (ApiOpenGL.IsChecked.Value)
				startInfo.Arguments += " --api opengl";
			if (ApiVulkan.IsChecked.Value)
				startInfo.Arguments += " --api vulkan";

			if (isFinished)
				startInfo.Arguments += " --finished";

			try
			{
				Process.Start(startInfo);
				Close();
				return true;
			}
			catch
			{
				return false;
			}
		}

		void InstallationStep0()
		{
			if (!isElevated && !IsWritable(Path.GetDirectoryName(targetPath)))
			{
				RestartWithElevatedPrivileges();
			}
			else
			{
				InstallationStep1();
			}
		}
		void InstallationStep1()
		{
			ApiGroup.IsEnabled = true;
			SetupButton.IsEnabled = false;
			ApiGroup.Visibility = ApiD3D9.Visibility = ApiDXGI.Visibility = ApiOpenGL.Visibility = ApiVulkan.Visibility = Visibility.Visible;
			ApiVulkanGlobal.Visibility = ApiVulkanGlobalButton.Visibility = Visibility.Collapsed;

			var info = FileVersionInfo.GetVersionInfo(targetPath);
			targetName = info.FileDescription;
			if (targetName is null || targetName.Trim().Length == 0)
				targetName = Path.GetFileNameWithoutExtension(targetPath);

			UpdateStatus("Working on " + targetName + " ...", "Analyzing executable ...");

			var peInfo = new PEInfo(targetPath);
			is64Bit = peInfo.Type == PEInfo.BinaryType.IMAGE_FILE_MACHINE_AMD64;

			// Check whether the API is specified in the compatibility list, in which case setup can continue right away
			var executableName = Path.GetFileName(targetPath);
			if (compatibilityIni != null && compatibilityIni.HasValue(executableName, "RenderApi"))
			{
				string api = compatibilityIni.GetString(executableName, "RenderApi");

				ApiD3D9.IsChecked = api == "D3D8" || api == "D3D9";
				ApiDXGI.IsChecked = api == "D3D10" || api == "D3D11" || api == "D3D12" || api == "DXGI";
				ApiOpenGL.IsChecked = api == "OpenGL";
				ApiVulkan.IsChecked = api == "Vulkan";

				InstallationStep2();
				return;
			}

			bool isApiD3D8 = peInfo.Modules.Any(s => s.StartsWith("d3d8", StringComparison.OrdinalIgnoreCase));
			bool isApiD3D9 = isApiD3D8 || peInfo.Modules.Any(s => s.StartsWith("d3d9", StringComparison.OrdinalIgnoreCase));
			bool isApiDXGI = peInfo.Modules.Any(s => s.StartsWith("dxgi", StringComparison.OrdinalIgnoreCase) || s.StartsWith("d3d1", StringComparison.OrdinalIgnoreCase) || s.Contains("GFSDK")); // Assume DXGI when GameWorks SDK is in use
			bool isApiOpenGL = peInfo.Modules.Any(s => s.StartsWith("opengl32", StringComparison.OrdinalIgnoreCase));
			bool isApiVulkan = peInfo.Modules.Any(s => s.StartsWith("vulkan-1", StringComparison.OrdinalIgnoreCase));

			if (isApiD3D9 && isApiDXGI)
			{
				isApiD3D9 = false; // Prefer DXGI over D3D9
			}
			if (isApiD3D8 && !isHeadless)
			{
				MessageBox.Show(this, "It looks like the target application uses Direct3D 8. You'll have to download an additional wrapper from 'https://github.com/crosire/d3d8to9/releases' which converts all API calls to Direct3D 9 in order to use ReShade.", "Warning", MessageBoxButton.OK, MessageBoxImage.Warning);
			}
			if (isApiDXGI && isApiVulkan)
			{
				isApiDXGI = false; // Prefer Vulkan over Direct3D 12
			}
			if (isApiOpenGL && (isApiD3D8 || isApiD3D9 || isApiDXGI || isApiVulkan))
			{
				isApiOpenGL = false; // Prefer Vulkan and Direct3D over OpenGL
			}

			Message.Text = "Which rendering API does " + targetName + " use?";

			ApiD3D9.IsChecked = isApiD3D9;
			ApiDXGI.IsChecked = isApiDXGI;
			ApiOpenGL.IsChecked = isApiOpenGL;
			ApiVulkan.IsChecked = isApiVulkan;
		}
		void InstallationStep2()
		{
			ApiGroup.IsEnabled = false;
			SetupButton.IsEnabled = false;
			ApiGroup.Visibility = ApiD3D9.Visibility = ApiDXGI.Visibility = ApiOpenGL.Visibility = ApiVulkan.Visibility = Visibility.Visible;
			ApiVulkanGlobal.Visibility = ApiVulkanGlobalButton.Visibility = Visibility.Collapsed;

			UpdateStatus("Working on " + targetName + " ...", "Installing ReShade ...");

			string targetDir = Path.GetDirectoryName(targetPath);
			configPath = Path.Combine(targetDir, "ReShade.ini");

			if (ApiVulkan.IsChecked != true)
			{
				modulePath = null;
				if (ApiD3D9.IsChecked == true)
				{
					modulePath = "d3d9.dll";
				}
				if (ApiDXGI.IsChecked == true)
				{
					modulePath = "dxgi.dll";
				}
				if (ApiOpenGL.IsChecked == true)
				{
					modulePath = "opengl32.dll";
				}

				if (modulePath == null)
				{
					// No API selected, abort immediately
					return;
				}

				modulePath = Path.Combine(targetDir, modulePath);

				var alternativeConfigPath = Path.ChangeExtension(modulePath, ".ini");
				if (File.Exists(alternativeConfigPath))
				{
					configPath = alternativeConfigPath;
				}

				if (File.Exists(modulePath) && !isHeadless)
				{
					var moduleInfo = FileVersionInfo.GetVersionInfo(modulePath);
					if (moduleInfo.ProductName == "ReShade")
					{
						ApiGroup.Visibility = Visibility.Collapsed;
						InstallButtons.Visibility = Visibility.Visible;

						Message.Text = "Existing ReShade installation found. How do you want to proceed?";

						// Do not hide exit button when asking about existing installation, so user can abort installation
						AeroGlass.HideSystemMenu(this, false);
					}
					else
					{
						UpdateStatusAndFinish(false, Path.GetFileName(modulePath) + " already exists, but does not belong to ReShade.", "Please make sure this is not a system file required by the game.");
					}
					return;
				}
			}

			InstallationStep3();
		}
		void InstallationStep3()
		{
			ApiGroup.Visibility = Visibility.Visible;
			InstallButtons.Visibility = Visibility.Collapsed;

			UpdateStatus("Working on " + targetName + " ...", "Installing ReShade ...");

			if (ApiVulkan.IsChecked != true)
			{
				try
				{
					var module = zip.GetEntry(is64Bit ? "ReShade64.dll" : "ReShade32.dll");
					if (module == null)
					{
						throw new FileFormatException("Expected ReShade archive to contain ReShade DLLs");
					}

					using (Stream input = module.Open())
					using (FileStream output = File.Create(modulePath))
					{
						input.CopyTo(output);
					}
				}
				catch (Exception ex)
				{
					UpdateStatusAndFinish(false, "Failed to install " + Path.GetFileName(modulePath) + ".", ex.Message);
					return;
				}

				// Create a default log file for troubleshooting
				File.WriteAllText(Path.ChangeExtension(modulePath, ".log"), @"
If you are reading this after launching the game at least once, it likely means ReShade was not loaded by the game.

In that event here are some steps you can try to resolve this:

1) Make sure this file and the related DLL are really in the same directory as the game executable.
   If that is the case and it does not work regardless, check if there is a 'bin' directory, move them there and try again.

2) Try running the game with elevated user permissions by doing a right click on its executable and choosing 'Run as administrator'.

3) If the game crashes, try disabling all game overlays (like Origin), recording software (like Fraps), FPS displaying software,
   GPU overclocking and tweaking software and other proxy DLLs (like ENB, Helix or Umod).

4) If none of the above helps, you can get support on the forums at https://forum.reshade.me. But search for your problem before
   creating a new topic, as somebody else may have already found a solution.
");
			}

			// Copy potential pre-made configuration file to target
			if (File.Exists("ReShade.ini") && !File.Exists(configPath))
			{
				try
				{
					File.Copy("ReShade.ini", configPath);
				}
				catch (Exception ex)
				{
					UpdateStatusAndFinish(false, "Failed to install " + Path.GetFileName(configPath) + ".", ex.Message);
					return;
				}
			}

			// Add default configuration
			var config = new IniFile(configPath);
			if (compatibilityIni != null && !config.HasValue("GENERAL", "PreprocessorDefinitions"))
			{
				config.SetValue("GENERAL", "PreprocessorDefinitions",
					"RESHADE_DEPTH_LINEARIZATION_FAR_PLANE=1000.0",
					"RESHADE_DEPTH_INPUT_IS_UPSIDE_DOWN=" + compatibilityIni.GetString(targetName, "DepthUpsideDown", "0"),
					"RESHADE_DEPTH_INPUT_IS_REVERSED=" + compatibilityIni.GetString(targetName, "DepthReversed", "0"),
					"RESHADE_DEPTH_INPUT_IS_LOGARITHMIC=" + compatibilityIni.GetString(targetName, "DepthLogarithmic", "0"));

				if (compatibilityIni.HasValue(targetName, "DepthCopyBeforeClears") ||
					compatibilityIni.HasValue(targetName, "UseAspectRatioHeuristics"))
				{
					switch (compatibilityIni.GetString(targetName, "RenderApi"))
					{
						case "D3D9":
							config.SetValue("DX9_BUFFER_DETECTION", "PreserveDepthBuffer", compatibilityIni.GetString(targetName, "DepthCopyBeforeClears", "0"));
							config.SetValue("DX9_BUFFER_DETECTION", "UseAspectRatioHeuristics", compatibilityIni.GetString(targetName, "UseAspectRatioHeuristics", "1"));
							break;
						case "D3D10":
							config.SetValue("DX10_BUFFER_DETECTION", "DepthBufferRetrievalMode", compatibilityIni.GetString(targetName, "DepthCopyBeforeClears", "0"));
							config.SetValue("DX10_BUFFER_DETECTION", "UseAspectRatioHeuristics", compatibilityIni.GetString(targetName, "UseAspectRatioHeuristics", "1"));
							break;
						case "D3D11":
							config.SetValue("DX11_BUFFER_DETECTION", "DepthBufferRetrievalMode", compatibilityIni.GetString(targetName, "DepthCopyBeforeClears", "0"));
							config.SetValue("DX11_BUFFER_DETECTION", "UseAspectRatioHeuristics", compatibilityIni.GetString(targetName, "UseAspectRatioHeuristics", "1"));
							break;
						case "D3D12":
							config.SetValue("DX12_BUFFER_DETECTION", "DepthBufferRetrievalMode", compatibilityIni.GetString(targetName, "DepthCopyBeforeClears", "0"));
							config.SetValue("DX12_BUFFER_DETECTION", "UseAspectRatioHeuristics", compatibilityIni.GetString(targetName, "UseAspectRatioHeuristics", "1"));
							break;
					}
				}

				config.SaveFile();
			}

			// Only show the selection dialog if there are actually packages to choose
			if (!isHeadless && packagesIni != null && packagesIni.GetSections().Length != 0)
			{
				var dlg = new SelectEffectsDialog(packagesIni);
				dlg.Owner = this;

				if (dlg.ShowDialog() == true)
				{
					var packages = new Queue<EffectPackage>(dlg.EnabledPackages);

					if (packages.Count != 0)
					{
						InstallationStep4(packages);
						return;
					}
				}
			}

			// Add default search paths if no config exists
			if (!config.HasValue("GENERAL", "EffectSearchPaths") && !config.HasValue("GENERAL", "TextureSearchPaths"))
			{
				WriteSearchPaths(".\\", ".\\");
			}

			InstallationStep6();
		}
		void InstallationStep4(Queue<EffectPackage> packages)
		{
			ApiGroup.IsEnabled = false;
			SetupButton.IsEnabled = false;
			ApiGroup.Visibility = ApiD3D9.Visibility = ApiDXGI.Visibility = ApiOpenGL.Visibility = ApiVulkan.Visibility = Visibility.Visible;
			ApiVulkanGlobal.Visibility = ApiVulkanGlobalButton.Visibility = Visibility.Collapsed;

			var package = packages.Dequeue();
			var downloadPath = Path.GetTempFileName();

			UpdateStatus("Working on " + targetName + " ...", "Downloading " + package.PackageName + " ...", package.DownloadUrl);

			// Add support for TLS 1.2, so that HTTPS connection to GitHub succeeds
			ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;

			var client = new WebClient();

			client.DownloadFileCompleted += (object sender, System.ComponentModel.AsyncCompletedEventArgs e) => {
				if (e.Error != null)
				{
					UpdateStatusAndFinish(false, "Failed to download from " + package.DownloadUrl + ".", e.Error.Message);
				}
				else
				{
					InstallationStep5(downloadPath, package);

					if (packages.Count != 0)
					{
						InstallationStep4(packages);
					}
				}
			};

			client.DownloadProgressChanged += (object sender, DownloadProgressChangedEventArgs e) => {
				// Avoid negative percentage values
				if (e.TotalBytesToReceive > 0)
				{
					Message.Text = "Downloading " + package.PackageName + " ... (" + ((100 * e.BytesReceived) / e.TotalBytesToReceive) + "%)";
				}
			};

			try
			{
				client.DownloadFileAsync(new Uri(package.DownloadUrl), downloadPath);
			}
			catch (Exception ex)
			{
				UpdateStatusAndFinish(false, "Failed to download from " + package.DownloadUrl + ".", ex.Message);
			}
		}
		void InstallationStep5(string downloadPath, EffectPackage package)
		{
			UpdateStatus("Working on " + targetName + " ...", "Extracting " + package.PackageName + " ...");

			string tempPath = Path.Combine(Path.GetTempPath(), "reshade-shaders");

			try
			{
				if (Directory.Exists(tempPath)) // Delete existing directories since extraction fails if the target is not empty
				{
					Directory.Delete(tempPath, true);
				}

				ZipFile.ExtractToDirectory(downloadPath, tempPath);

				// First check for a standard folder name
				string tempPathShaders = Directory.GetDirectories(tempPath, "Shaders", SearchOption.AllDirectories).FirstOrDefault();
				string tempPathTextures = Directory.GetDirectories(tempPath, "Textures", SearchOption.AllDirectories).FirstOrDefault();

				// If that does not exist, look for the first directory that contains shaders/textures
				if (tempPathShaders == null)
				{
					tempPathShaders = Directory.GetFiles(tempPath, "*.fx", SearchOption.AllDirectories).Select(x => Path.GetDirectoryName(x)).OrderBy(x => x.Length).FirstOrDefault();
				}
				if (tempPathTextures == null)
				{
					tempPathTextures = Directory.GetFiles(tempPath, "*.png", SearchOption.AllDirectories).Select(x => Path.GetDirectoryName(x)).OrderBy(x => x.Length).FirstOrDefault();
				}

				// Move only the relevant files to the target
				if (tempPathShaders != null)
				{
					MoveFiles(tempPathShaders, Path.Combine(Path.GetDirectoryName(targetPath), package.InstallPath));
				}
				if (tempPathTextures != null)
				{
					MoveFiles(tempPathTextures, Path.Combine(Path.GetDirectoryName(targetPath), package.TextureInstallPath));
				}

				File.Delete(downloadPath);
				Directory.Delete(tempPath, true);
			}
			catch (Exception ex)
			{
				UpdateStatusAndFinish(false, "Failed to extract " + package.PackageName + ".", ex.Message);
				return;
			}

			WriteSearchPaths(package.InstallPath, package.TextureInstallPath);

			InstallationStep6();
		}
		void InstallationStep6()
		{
			string description = null;
			if (ApiVulkan.IsChecked.Value)
			{
				description = "You need to keep this setup tool open for ReShade to work in Vulkan games!\nAlternatively check the option below:";

				ApiGroup.IsEnabled = true;
				ApiD3D9.Visibility = ApiDXGI.Visibility = ApiOpenGL.Visibility = ApiVulkan.Visibility = Visibility.Collapsed;
				ApiVulkanGlobal.Visibility = Visibility.Visible;

				if (isElevated)
				{
					ApiGroup.Visibility = Visibility.Visible;
					ApiVulkanGlobalButton.Visibility = Visibility.Collapsed;
				}
				else
				{
					ApiGroup.Visibility = Visibility.Collapsed;
					ApiVulkanGlobalButton.Visibility = Visibility.Visible;
				}
			}
			else
			{
				description = "You may now close this setup tool or click this button to edit additional settings.";
			}

			UpdateStatusAndFinish(true, "Edit ReShade settings", description);

			SetupButton.IsEnabled = true;
		}

		void OnWindowInit(object sender, EventArgs e)
		{
			AeroGlass.HideIcon(this);
			AeroGlass.ExtendFrame(this);
		}
		void OnWindowLoaded(object sender, RoutedEventArgs e)
		{
			var args = Environment.GetCommandLineArgs().Skip(1).ToArray();

			bool hasApi = false;
			bool hasFinished = false;

			// Parse command line arguments
			for (int i = 0; i < args.Length; i++)
			{
				if (args[i] == "--headless")
				{
					isHeadless = true;
					continue;
				}
				if (args[i] == "--elevated")
				{
					isElevated = true;
					continue;
				}
				if (args[i] == "--finished")
				{
					hasFinished = true;
					continue;
				}

				if (i + 1 < args.Length)
				{
					if (args[i] == "--api")
					{
						hasApi = true;

						string api = args[++i];
						ApiD3D9.IsChecked = api == "d3d9";
						ApiDXGI.IsChecked = api == "dxgi" || api == "d3d10" || api == "d3d11";
						ApiOpenGL.IsChecked = api == "opengl";
						ApiVulkan.IsChecked = api == "vulkan";
						continue;
					}

					if (args[i] == "--top")
					{
						Top = double.Parse(args[++i]);
						continue;
					}
					if (args[i] == "--left")
					{
						Left = double.Parse(args[++i]);
						continue;
					}
				}

				if (File.Exists(args[i]))
				{
					targetPath = args[i];
					targetName = Path.GetFileNameWithoutExtension(targetPath);
					configPath = Path.Combine(Path.GetDirectoryName(targetPath), "ReShade.ini");
				}
			}

			if (targetPath != null)
			{
				if (hasFinished)
				{
					InstallationStep6();
				}
				else if (hasApi)
				{
					InstallationStep2();
				}
				else
				{
					InstallationStep1();
				}
				return;
			}

			if (!ApiVulkanGlobal.IsChecked.Value)
			{
				// Enable Vulkan layer while the setup tool is running
				EnableVulkanLayer(Registry.CurrentUser);
			}
			else
			{
				// Update existing Vulkan layer with the included version
				EnableVulkanLayer(Registry.LocalMachine);
			}
		}
		void OnWindowClosed(object sender, EventArgs e)
		{
			if (!ApiVulkanGlobal.IsChecked.Value)
			{
				// Disable Vulkan layer again when the setup tool is being closed
				DisableVulkanLayer(Registry.CurrentUser);
			}
		}

		void OnApiChecked(object sender, RoutedEventArgs e)
		{
			InstallationStep2();
		}
		void OnApiVulkanGlobalChecked(object sender, RoutedEventArgs e)
		{
			if (sender is System.Windows.Controls.Button button)
			{
				RestartWithElevatedPrivileges();
				return;
			}

			var checkbox = sender as System.Windows.Controls.CheckBox;
			if (checkbox.IsChecked == IsVulkanLayerEnabled(Registry.LocalMachine))
			{
				return;
			}

			// Should have elevated privileges at this point
			if (!isElevated)
			{
				// Reset check box to previous value if unable to get elevated privileges
				checkbox.IsChecked = !checkbox.IsChecked;
				return;
			}

			// Switch between installing to HKLM and HKCU based on check box value
			if (checkbox.IsChecked.Value)
			{
				DisableVulkanLayer(Registry.CurrentUser);
				if (!EnableVulkanLayer(Registry.LocalMachine))
				{
					UpdateStatusAndFinish(false, "Failed to install global Vulkan layer.");
					checkbox.IsChecked = !checkbox.IsChecked;
				}
			}
			else
			{
				DisableVulkanLayer(Registry.LocalMachine);
				if (!EnableVulkanLayer(Registry.CurrentUser))
				{
					UpdateStatusAndFinish(false, "Failed to install user local Vulkan layer.");
					checkbox.IsChecked = !checkbox.IsChecked;
				}
			}
		}

		void OnUpdateClick(object sender, RoutedEventArgs e)
		{
			InstallationStep3();
		}
		void OnUninstallClick(object sender, RoutedEventArgs e)
		{
			ApiGroup.Visibility = Visibility.Visible;
			InstallButtons.Visibility = Visibility.Collapsed;

			try
			{
				string targetDir = Path.GetDirectoryName(targetPath);

				File.Delete(modulePath);

				if (File.Exists(configPath))
				{
					File.Delete(configPath);
				}

				if (File.Exists(Path.ChangeExtension(modulePath, ".log")))
				{
					File.Delete(Path.ChangeExtension(modulePath, ".log"));
				}

				if (Directory.Exists(Path.Combine(targetDir, "reshade-shaders")))
				{
					Directory.Delete(Path.Combine(targetDir, "reshade-shaders"), true);
				}

				UpdateStatusAndFinish(true, "Successfully uninstalled.");
			}
			catch (Exception ex)
			{
				UpdateStatusAndFinish(false, "Failed to delete some ReShade files.", ex.Message);
			}
		}

		void OnSetupButtonClick(object sender, RoutedEventArgs e)
		{
			if (isFinished)
			{
				new SettingsDialog(configPath) { Owner = this }.ShowDialog();
				return;
			}

			if (targetPath == null && Keyboard.Modifiers == ModifierKeys.Control)
			{
				try
				{
					zip.ExtractToDirectory(".");
				}
				catch (Exception ex)
				{
					UpdateStatusAndFinish(false, "Failed to extract some ReShade files.", ex.Message);
					return;
				}

				Close();
				return;
			}

			if (Keyboard.Modifiers == ModifierKeys.Alt)
			{
				var dlg = new OpenFileDialog
				{
					Filter = "Applications|*.exe",
					DefaultExt = ".exe",
					Multiselect = false,
					ValidateNames = true,
					CheckFileExists = true
				};

				if (dlg.ShowDialog(this) == true)
				{
					targetPath = dlg.FileName;

					InstallationStep0();
				}
			}
			else
			{
				var dlg = new SelectAppDialog();
				dlg.Owner = this;

				if (dlg.ShowDialog() == true)
				{
					targetPath = dlg.FileName;

					InstallationStep0();
				}
			}
		}
		void OnSetupButtonDragDrop(object sender, DragEventArgs e)
		{
			if (targetPath != null)
			{
				// Prevent drag and drop if another installation is already in progress
				return;
			}

			if (e.Data.GetData(DataFormats.FileDrop, true) is string[] files && files.Length >= 1)
			{
				targetPath = files[0];

				InstallationStep0();
			}
		}
	}
}
