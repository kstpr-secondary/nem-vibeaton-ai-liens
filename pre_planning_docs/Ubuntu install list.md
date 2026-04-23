# Ubuntu install list:

## Update to latest:
```
sudo apt update
sudo apt upgrade
sudo apt dist-ugrade
sudo apt autoremove
```

## Latest NVidia drivers (580)

Confirm with `nvidia-smi`

## Swappiness 10 - less dumping of RAM contents to SSD 
```
sudo nano /etc/sysctl.conf
Add `swappiness=10` at the end of the file
CTRL+O -> save CTRL + X to exit nano
```

## VS Code Insiders snap or deb

https://code.visualstudio.com/insiders

## Slack snap

https://snapcraft.io/slack

## Kitty apt (rich terminal emulator, run CLIs there)
```
sudo apt update && sudo apt install kitty
```

## Node (from nodesource, needed to install Gemini)
**DON'T** download from apt, version is stale there, do the following:
```
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
sudo apt install -y nodejs
node --version   # should print v22.x.x
```

## Gemini
```
npm install -g @google/gemini-cli
```

## Claude Code
```
curl -fsSL https://claude.ai/install.sh | bash
```

## Copilot CLI
```
curl -fsSL https://gh.io/copilot-install | bash
```

## C++, Vulkan and Other Graphics Dependencies
```
sudo apt install -y \
  clang cmake ninja-build pkg-config git \
  xorg-dev libx11-dev libxi-dev libxcursor-dev libxrandr-dev libxinerama-dev \
  libgl1-mesa-dev mesa-common-dev \
  libvulkan-dev vulkan-tools vulkan-validationlayers spirv-tools glslang-tools
```

---

gemini update --quiet - по-малко junk в output-а



cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build

If you want the per-workstream targets instead of a full build:

 cmake --build build --target renderer_app renderer_tests
 cmake --build build --target engine_app engine_tests
 cmake --build build --target game

---

Download Sokol shader compiler:
https://github.com/floooh/sokol-tools-bin/tree/master/bin/linux