library example_build;

import 'dart:io';
import 'package:args/args.dart';
import 'package:ccompile/ccompile.dart';

void main() {
  build();
}

void build() {

  var config = 'extension.yaml';
  
  var args = _processArgs(new Options().arguments);
  
  var changedFiles = args["changed"];
  var removedFiles = args["removed"];
  var cleanBuild = args["clean"];
  
  var mustCompile = changedFiles.some((f) => f.endsWith(".cc"));
  
  if (!mustCompile) { return; }
  
  var rootPath = new File(new Options().script).directorySync().path;
  var workingDirectory = "$rootPath/lib";
  
  var projectName = '$workingDirectory/$config';

  var builder = new ProjectBuilder();
  var errors = [];
  builder.loadProject(projectName).then((project) {
    SystemUtils.writeStdout('Building project "$projectName"');

    builder.buildAndClean(project, workingDirectory).then((result) {
      if(result.exitCode != 0) {
        SystemUtils.writeStdout('Error building project.');
        SystemUtils.writeStdout('Exit code: ${result.exitCode}');
        if(!result.stdout.isEmpty) {
          SystemUtils.writeStdout(result.stdout);
        }

        if(!result.stderr.isEmpty) {
          SystemUtils.writeStderr(result.stderr);
        }
        exit(result.exitCode);
      } else {
        SystemUtils.writeStdout('Project is built successfully.');
      }
    });
  });

}


ArgResults _processArgs(List<String> arguments) {
  var parser = new ArgParser()
    ..addOption("changed", help: "the file has changed since the last build",
        allowMultiple: true)
    ..addOption("removed", help: "the file was removed since the last build",
        allowMultiple: true)
    ..addFlag("clean", negatable: false, help: "remove any build artifacts")
    ..addFlag("help", negatable: false, help: "displays this help and exit");
  var args = parser.parse(arguments);
  if (args["help"]) {
    print(parser.getUsage());
    exit(0);
  }
  return args;
}
