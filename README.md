# General Information
This is the source code for https://pekora.org revival multi-client bootstrapper.
- **why?:** Because several people were saying that pekora is a **RAT (Remote Access Trojan)**, by open sourcing we are proving ourselves that we aren't a RAT and there's nothing to worry about.

# How to build

### Requirements:
1. **Visual Studio 2022**
2. **[vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-vs?pivots=shell-powershell)**.

### Steps to Build The Bootstrapper:
1. Clone the repository with **submodules**
   ``git clone --recurse-submodules https://github.com/kerosina/Pekora-Bootstrapper.git``
2. Navigate into the ``vcpkg`` folder
3. Run ``bootstrap-vcpkg.bat``
4. Once that's done, run ``vcpkg integrate install``
5. **Open the solution** in **Visual Studio**.
6. **Set the configuration** to **Release | x86** or **Release | x64**.
7. **Build BootstrapperClient**.
8. **Done**
