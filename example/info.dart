library lesson1;

// TODO - Dart doesn't like package: with native extensions
import 'packages/webcl/webcl.dart' as WebCL;

main() {
  try {

    WebCL.platforms.forEach((platform) {
      
      print("${platform.name}\n"
            "vendor: ${platform.vendor}\n"
            "version: ${platform.version}\n"
            "profile: ${platform.profile}\n"
            "extensions: ${platform.extensions}\n");
      
      platform.allDevices.forEach((device){
        print("device: ${device.name}");
      });
    });
    
    print("Excellent! Your system does support WebCL.");
  } catch (e) {
    print("[$e]\nUnfortunately platform or device inquiry failed.");
  }
}
