# Cangjie Language Server User Guide

## Feature Overview

The `Cangjie Language Server` provides language service functionalities such as definition jumping, reference finding, and code completion based on the Cangjie language.

## Usage Instructions

To use the Cangjie Language Server, it must be combined with the Openharmony SDK. Ensure that the `tools/bin` directory of the SDK contains the LSPServer binary for the Cangjie language service. Additionally, the DevEco Studio client is required.

The startup parameters for `Cangjie Language Server` are as follows:

```bash
--test                Used to launch LSPServer in test mode
--disableAutoImport   Disables the automatic package import feature for code completion
--enable-log=<value>  Disables log generation by default; pass true to open log generation
--log-path=<value>    Sets the path for generated logs
-V                    Enables crash log generation functionality
```