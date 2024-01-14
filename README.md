# Hyprslide

A program that creates a wallpaper slideshow using hyprpaper for hyprland desktop

## Building

```console
cc hyprslide.c -o hyprslide
```
You can -O3 the build if you want

## Usage

It is advised to add it to your hyprland.conf

Example:
```
exec-once = ~/Path/to/hyprslide -t 10 -r -p ~/dotfiles/Wallpapers -m eDP-1
```

The monitor path can be obtained by
```console
hyprctl monitors
```
