#!/usr/bin/env sh
GRADLE_APP_NAME="Gradle"
GRADLE_APP_BASE_NAME=`basename "$0"`
DEFAULT_JVM_OPTS='"-Xmx64m" "-Xms64m"'

die () {
    echo
    echo "$*"
    echo
    exit 1
} >&2

APP_HOME=`pwd -P`
CLASSPATH=$APP_HOME/gradle/wrapper/gradle-wrapper.jar

if ! [ -f "$CLASSPATH" ]; then
    die "ERROR: gradle-wrapper.jar not found."
fi

JAVACMD="java"

exec "$JAVACMD" $DEFAULT_JVM_OPTS $JAVA_OPTS $GRADLE_OPTS \
    "-Dorg.gradle.appname=$GRADLE_APP_BASE_NAME" \
    -classpath "$CLASSPATH" \
    org.gradle.wrapper.GradleWrapperMain \
    "$@"
