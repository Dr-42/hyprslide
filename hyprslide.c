#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <signal.h>

#define DR42_TRACE_IMPLEMENTATION
#include "trace.h"

typedef struct image_info_t{
	char** paths;
	int count;
} image_info_t;

// Allocates memory
image_info_t get_images_in_dir(char* dir){
	int count = 0;
	DIR* d;
	struct dirent* dirthing;

	d = opendir(dir);
	if(d){
		while((dirthing = readdir(d)) != NULL){
			if(dirthing->d_type == DT_REG){
				count++;
			}
		}
		closedir(d);
	}

	char** images = malloc(count * sizeof(char*));
	for(int i = 0; i < count; i++){
		images[i] = malloc(256 * sizeof(char));
	}

	d = opendir(dir);
	if(d){
		int i = 0;
		while((dirthing = readdir(d)) != NULL){
			if(dirthing->d_type == DT_REG){
				strcpy(images[i], dirthing->d_name);
				i++;
			}
		}
		closedir(d);
	}
	image_info_t info;
	info.paths = images;
	info.count = count;
	return info;
}

void free_images(image_info_t images){
	for(int i = 0; i < images.count; i++){
		free(images.paths[i]);
	}
	free(images.paths);
}

void sig_handler(int signo){
	(void)signo;
	print_trace();
	exit(1);
}

int main(int argc, char **argv){
	signal(SIGSEGV, sig_handler);
	// Defaults
	int time = 60;
	int random = 0;
	char* dir;
	char* monitor;

	// Parse arguments
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "-h") == 0){
			printf("hyprslide - A simple wallpaper slideshow program for hyprland and hyprpaper\n");
			printf("Usage: hyprslide [options]\n");
			printf("Options:\n");
			printf("\t-h\t\tShow this help message\n");
			printf("\t-t <time>\tSet time between slides (in seconds)\n");
			printf("\t-r\t\tRandomize slide order\n");
			printf("\t-p <path>\tSet path to images\n");
			printf("\t-m <monitor>\tSet monitor to display slides on\n");
			return 0;
		}
		else if(strcmp(argv[i], "-t") == 0){
			i++;
			time = atoi(argv[i]);
			printf("Time: %d\n", time);
		}
		else if(strcmp(argv[i], "-r") == 0){
			random = 1;
			printf("Random: %d\n", random);
		}
		else if(strcmp(argv[i], "-p") == 0){
			i++;
			dir = argv[i];
			// If path ends with a slash, remove it
			if(dir[strlen(dir) - 1] == '/'){
				dir[strlen(dir) - 1] = '\0';
			}
			printf("Path: %s\n", dir);
		}
		else if(strcmp(argv[i], "-m") == 0){
			i++;
			monitor = argv[i];
			printf("Monitor: %s\n", monitor);
		}
		else{
			printf("Invalid argument: %s\n", argv[1]);
			return 1;
		}
	}


	const char* home = getenv("HOME");
	const char* conf_path = "/.config/hypr/hyprpaper.conf";
	const char* backup_path = "/.config/hypr/hyprpaper.conf.bak";

	char* old = malloc(strlen(home) + strlen(conf_path) + 1);
	strcpy(old, home);
	strcat(old, conf_path);

	char* new = malloc(strlen(home) + strlen(backup_path) + 1);
	strcpy(new, home);
	strcat(new, backup_path);

	// Check if config file exists
	if(access(old, F_OK) != -1){
		if(access(new, F_OK) != -1){
			remove(new);
		}
		rename(old, new);
	}


	image_info_t image_info = get_images_in_dir(dir);
	if(image_info.count == 0){
		printf("No images found in %s\n", dir);
		return 1;
	}

	// Create config file
	printf("Creating config file at %s\n", old);
	FILE* fp = fopen(old, "w");
	if(fp == NULL){
		printf("Error creating config file\n");
		return 1;
	}
	for(int i = 0; i < image_info.count; i++){
		printf("Adding %s to config file\n", image_info.paths[i]);
		fprintf(fp, "preload = %s/%s\n", dir, image_info.paths[i]);
	}
	fclose(fp);
	free(old);
	free(new);

	int index = 0;

	// Check if hyprpaper is running
	if(system("pidof -x hyprpaper > /dev/null") == 0){
		system("pkill hyprpaper");
		system("hyprpaper &");
	} else {
		system("hyprpaper &");
	}
	while(1){
		if(random){
			index = rand() % image_info.count;
		}
		else{
			index = (index + 1) % image_info.count;
		}

		char command[256];
		sprintf(command, "hyprctl hyprpaper wallpaper %s,%s/%s", monitor, dir, image_info.paths[index]);
		system(command);
		sleep(time);
	}
	free_images(image_info);
	return 0;
}
