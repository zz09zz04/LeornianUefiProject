# Leornian UEFI Project Package

## What is Leornian?

**Leornian** is the codename of this UEFI firmware project. The name originates from the Old English word *leornian*, which means **"to learn, to gain knowledge, and to grow"**.

In this project, **Leornian** represents the continuous evolution of firmware technology through learning, innovation, and improvement. It reflects the project's goal of building a reliable and maintainable UEFI firmware platform while continuously accumulating engineering knowledge and experience.

This package is based on the EDK II build infrastructure with project-specific extensions and automation scripts. It includes the project source code, build scripts, configuration files, and the required development environment for building and maintaining the firmware.

---

# Prerequisites

## 1. Build Environment

Leornian is based on **EDK II (edk2-stable202605)** currently.

Install the following tools before building the project.

| Tool                    | Required Version                 |
|-------------------------|----------------------------------|
| Microsoft Visual Studio | Visual Studio 2022               |
| Python                  | 3.10 or later (3.12 recommended) |

EDK II officially supports Microsoft Visual Studio toolchains and recommends using the Python version specified in the EDK II repository for maximum compatibility.

> **Recommendation**
>
> Install the latest supported versions of Visual Studio and Python unless your project requires a specific toolchain.

---

# Repository Setup

## 2. Initialize Git Submodules

After cloning the repository, execute cmd batch:

```bat
submodule_update.bat
```

This script automatically initializes and updates all required Git submodules while ignoring initialization failures to allow the setup process to continue.

Equivalent the following git bash command:

```bash
git submodule status | awk '{print $2}' | while read sub; do
    echo "Updating $sub"
    git submodule update --init --recursive "$sub" || echo "Skip $sub"
done
```

---

# Building the Project

## 3. Build Command

The project is built using the provided build script.

Change directory to project folder:

```bat
cd Platform\LeornianProjectPkg
```

and execute build batch

```bat
ProjectBuild.bat [RELEASE|DEBUG] [FLAG1] [FLAG2=VALUE] ...
```

### Parameters

| Parameter     | Description                           |
|---------------|---------------------------------------|
| `RELEASE`     | Build a release image                 |
| `DEBUG`       | Build a debug image                   |
| `FLAG1`       | Enable a project-specific feature     |
| `FLAG2=VALUE` | Assign a value to a build option      |

### Examples

Build a release image:

```bat
ProjectBuild.bat RELEASE
```

Build a debug image:

```bat
ProjectBuild.bat DEBUG
```

Build with additional options:

```bat
ProjectBuild.bat RELEASE "ANSI_COLOR_DEBUG=FALSE" "ACPIVIEW_ENABLE=TRUE" "QEMU_SUPPORT=TRUE"
```

---

# Build Output

After a successful build, the generated firmware binaries can be found in the project's output directory (for example, the `Build` directory), depending on the selected build target and platform configuration.