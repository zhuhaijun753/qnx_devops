# Safety Microprocessor Unit (Cortex-A53)

## Build status

*to be defined*

## Content

This repository provides the source code for the quad **Arm Cortex-A53** cores which are part of the **NXP S32G Safe and Secure Vehicle Network Processors** on the ProAI platform.

The Arm Cortex-A53 implements the [ARMv8-A](https://static.docs.arm.com/ddi0487/fa/DDI0487F_a_armv8_arm.pdf) 64bit architecture.

## Directory layout

The repository is structured by using the following directory layout:

Directory | Description                                                                                       | Usage
---       | ---                                                                                               | ---
`\build`  | Storage of build scripts, makefiles, build relevant tool configurations (e.g. linker definitions) | Not used yet
`\design` | Storage of files related to the software design (e.g. UML diagrams)                               | Not used yet
`\docs`   | Storage of software documentation bound to release                                                | Not used yet
`\output` | Storage of build results, hex files, etc. *(ignored)*                                             | Not used yet
`\repos`  | Mount point for Git submodules, e.g. precompiled libraries, a2l files, etc. *(ignored)*           | Not relevant
`\source` | Storage of source files (subdirectories can be created for structuring)                           | Not used yet
`\tests`  | Storage for module tests, test scripts, etc.                                                      | Not used yet
`\tools`  | Storage for additional project specific tools and (not build relevant) tool configurations        | Not used yet

## Build

Process the following steps in order to generate the build artifacts:

- *to be defined*

## Contacts

This repository is maintained by [Markus Kr�tz](mailto:markus.kroetz@zf.com).
