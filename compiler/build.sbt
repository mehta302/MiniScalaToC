name := "CMScala-compiler"

version := "1.0"

organization := "Purdue"


/**
 *  The main sbt configuration for the compiler build.
 */

lazy val defaults = Seq(
  // scala compiler version:
  scalaVersion := "2.12.18",
  scalaBinaryVersion := "2.12",
  scalacOptions ++= Seq("-feature", "-deprecation", "-unchecked"),

  // source location configuration:
  Compile / scalaSource := baseDirectory.value / "src",
  Test / scalaSource := baseDirectory.value / "test",
  Compile / resourceDirectory := baseDirectory.value / "resources",

  // eclipse configuration:
  Compile / unmanagedSourceDirectories := Seq((Compile / scalaSource).value),
  Test / unmanagedSourceDirectories := Seq((Test / scalaSource).value),

  // run configuration
  run / fork := false,
  run / connectInput := false,
  outputStrategy := Some(StdoutOutput),
  run / javaOptions ++= Seq("-Xss32M", "-Xms128M"),

  // test configuration:
  libraryDependencies += "com.novocode" % "junit-interface" % "0.11" % "test",
  testOptions += Tests.Argument(TestFrameworks.JUnit, "-q", "-v"),
  Test / parallelExecution := false,
  // fork in test
  Test / fork := true,
  //Test / javaOptions ++= Seq("-Djava.security.manager", "-Djava.security.policy=" + baseDirectory.value + "/project/tests.policy"),
  Test / javaOptions ++= Seq("-Xss32M", "-Xms128M"),

  // packaging configuration (also include tests, create submit-me.jar):
  Compile / packageSrc / unmanagedSources ++= (Test / unmanagedSources).value,
  Compile / packageSrc / mappings := {
    (Compile / packageSrc / unmanagedSources).value pair Path.relativeTo(baseDirectory.value)
  },
  Compile / packageSrc / artifactPath := submitMe.value,
  cleanFiles += submitMe.value
)

// The location desired for the source jar that will be submitted
def submitMe = baseDirectory(_ / "submit-me.jar")

lazy val cmScala = (project in file(".")).settings(
  defaults,
  name := "CMScala")
