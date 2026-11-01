# Read-only Windows output/session volume inspection. Does not play or change audio.
$ErrorActionPreference = 'Stop'
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace ReverseReverbDiagnostics {
 [ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")] class Enumerator {}
 [ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface IDevices {
  void EnumAudioEndpoints(int flow, uint mask, out IDeviceCollection devices);
  void GetDefaultAudioEndpoint(int flow, int role, out IDevice device);
 }
 [ComImport, Guid("0BD7A1BE-7A1A-44DB-8397-CC5392387B5E"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface IDeviceCollection { void GetCount(out uint count); void Item(uint index, out IDevice device); }
 [ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface IDevice {
  void Activate(ref Guid iid, uint context, IntPtr parameters, [MarshalAs(UnmanagedType.IUnknown)] out object obj);
  void OpenPropertyStore(uint access, out IntPtr properties);
  void GetId([MarshalAs(UnmanagedType.LPWStr)] out string id);
 }
 [ComImport, Guid("5CDF2C82-841E-4546-9722-0CF74078229A"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface IEndpointVolume {
  void RegisterControlChangeNotify(IntPtr p); void UnregisterControlChangeNotify(IntPtr p);
  void GetChannelCount(out uint count); void SetMasterVolumeLevel(float v, IntPtr context);
  void SetMasterVolumeLevelScalar(float v, IntPtr context); void GetMasterVolumeLevel(out float v);
  void GetMasterVolumeLevelScalar(out float v);
  void SetChannelVolumeLevel(uint c, float v, IntPtr context); void SetChannelVolumeLevelScalar(uint c, float v, IntPtr context);
  void GetChannelVolumeLevel(uint c, out float v); void GetChannelVolumeLevelScalar(uint c, out float v);
  void SetMute([MarshalAs(UnmanagedType.Bool)] bool mute, IntPtr context);
  void GetMute([MarshalAs(UnmanagedType.Bool)] out bool mute);
 }
 [ComImport, Guid("77AA99A0-1BD6-484F-8BC7-2C654C9A9B6F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface ISessions {
  void GetAudioSessionControl(IntPtr guid, uint flags, out IntPtr control);
  void GetSimpleAudioVolume(IntPtr guid, uint flags, out IntPtr volume);
  void GetSessionEnumerator(out ISessionEnumerator sessions);
 }
 [ComImport, Guid("E2F5BB11-0570-40CA-ACDD-3AA01277DEE8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface ISessionEnumerator { void GetCount(out int count); void GetSession(int index, out ISession session); }
 [ComImport, Guid("BFB7FF88-7239-4FC9-8FA2-07C950BE9C6D"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface ISession {
  void GetState(out int state); void GetDisplayName(out IntPtr name); void SetDisplayName(IntPtr name, IntPtr context);
  void GetIconPath(out IntPtr path); void SetIconPath(IntPtr path, IntPtr context);
  void GetGroupingParam(out Guid group); void SetGroupingParam(ref Guid group, IntPtr context);
  void RegisterAudioSessionNotification(IntPtr p); void UnregisterAudioSessionNotification(IntPtr p);
  void GetSessionIdentifier(out IntPtr id); void GetSessionInstanceIdentifier(out IntPtr id); void GetProcessId(out uint pid);
 }
 [ComImport, Guid("87CE5498-68D6-44E5-9215-6DA47EF883D8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
 interface IVolume {
  void SetMasterVolume(float v, IntPtr context); void GetMasterVolume(out float v);
  void SetMute([MarshalAs(UnmanagedType.Bool)] bool mute, IntPtr context);
  void GetMute([MarshalAs(UnmanagedType.Bool)] out bool mute);
 }
 public class Reading {
  public int Endpoint; public bool DefaultOutput; public string App; public int VolumePercent; public bool Muted; public bool Active;
 }
 public static class Audio {
  public static Reading[] Read() {
   var result = new List<Reading>();
   var enumerator = (IDevices) new Enumerator();
   IDevice defaultDevice; enumerator.GetDefaultAudioEndpoint(0, 1, out defaultDevice);
   string defaultId; defaultDevice.GetId(out defaultId);
   IDeviceCollection devices; enumerator.EnumAudioEndpoints(0, 1, out devices);
   uint count; devices.GetCount(out count);
   for (uint i = 0; i < count; ++i) {
    IDevice device; devices.Item(i, out device); string id; device.GetId(out id);
    var volumeId = typeof(IEndpointVolume).GUID; object endpointObject;
    device.Activate(ref volumeId, 23, IntPtr.Zero, out endpointObject);
    var endpoint = (IEndpointVolume) endpointObject; float level; bool mute;
    endpoint.GetMasterVolumeLevelScalar(out level); endpoint.GetMute(out mute);
    result.Add(new Reading { Endpoint=(int)i, DefaultOutput=id==defaultId, App="Windows output", VolumePercent=(int)Math.Round(level*100), Muted=mute, Active=true });
    var sessionsId = typeof(ISessions).GUID; object sessionsObject;
    device.Activate(ref sessionsId, 23, IntPtr.Zero, out sessionsObject);
    ISessionEnumerator sessions; ((ISessions)sessionsObject).GetSessionEnumerator(out sessions);
    int sessionCount; sessions.GetCount(out sessionCount);
    for (int j = 0; j < sessionCount; ++j) {
     ISession session; sessions.GetSession(j, out session); uint pid; int state;
     session.GetProcessId(out pid); session.GetState(out state);
     string app = "System sounds";
     if (pid != 0) { try { app=Process.GetProcessById((int)pid).ProcessName; } catch { app="Exited app"; } }
     var volume = (IVolume)session; volume.GetMasterVolume(out level); volume.GetMute(out mute);
     result.Add(new Reading { Endpoint=(int)i, DefaultOutput=id==defaultId, App=app, VolumePercent=(int)Math.Round(level*100), Muted=mute, Active=state==1 });
    }
   }
   return result.ToArray();
  }
 }
}
'@
[ReverseReverbDiagnostics.Audio]::Read() | ConvertTo-Json
