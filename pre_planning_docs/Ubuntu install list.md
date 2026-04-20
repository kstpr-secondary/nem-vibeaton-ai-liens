# Ubuntu install list:

## Update to latest:
```
sudo apt update
sudo apt upgrade
sudo apt dist-ugrade
sudo apt autoremove
```

## Latest NVidia drivers (580)

## Swappiness 10 - less dumping of RAM contents to SSD 
```
/etc/sysctl.conf -> swappiness=10
```

## VS Code Insiders snap

## Slack snap

## Kitty apt (rich terminal emulator, run CLIs there)
```
sudo apt update && sudo apt install kitty
```

## Node (from nodesource, needed to install Gemini)
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

## clang, cmake, ninja-build


---

gemini update --quiet - по-малко junk в output-а