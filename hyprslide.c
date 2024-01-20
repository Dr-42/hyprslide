#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

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

void print_help(){
	printf("hyprslide - A simple wallpaper slideshow program for hyprland and hyprpaper\n");
	printf("Usage: hyprslide [options]\n");
	printf("Options:\n");
	printf("\t-h\t\tShow this help message\n");
	printf("\t-t [time]\tSet time between slides (in seconds)\n");
	printf("\t\t\tDefault: 60\n");
	printf("\t-r\t\tRandomize slide order\n");
	printf("\t-p <path>\tSet path to images\n");
	printf("\t-m <monitor>\tSet monitor to display slides on\n");
}

int main(int argc, char **argv){
	srand(time(NULL));
	// Defaults
	int deftime = 60;
	int random = 0;
	char* dir = NULL;
	char* monitor = NULL;

	// Parse arguments
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "-h") == 0){
			print_help();
			return 0;
		}
		else if(strcmp(argv[i], "-t") == 0){
			i++;
			deftime = atoi(argv[i]);
			printf("Time: %d\n", deftime);
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

	if(dir == NULL){
		printf("No path specified\n");
		print_help();
		return 1;
	}

	if(monitor == NULL){
		printf("No monitor specified\n");
		print_help();
		return 1;
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
	if (random){
		fprintf(fp, "wallpaper = %s,%s/%s\n", monitor, dir, image_info.paths[rand() % image_info.count]);
	} else {
		fprintf(fp, "wallpaper = %s,%s\n", monitor, image_info.paths[0]);
	}
	fclose(fp);
	free(old);
	free(new);

	int index = 0;

	char line[1024];
	FILE *cmd = popen("pidof hyprpaper", "r");

	fgets(line, 1024, cmd);
	pid_t pid_old = strtoul(line, NULL, 10);

	pclose(cmd);

	if(pid_old != 0){
		printf("Killing old hyprpaper instance\n");
		kill(pid_old, SIGTERM);
	}

	int pid = fork();

	if(pid == 0){
		execlp("hyprpaper", "hyprpaper", NULL);
	}
	else if(pid < 0){
		printf("Error starting hyprpaper\n");
		return 1;
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
		sleep(deftime);
	}
	free_images(image_info);
	return 0;
}
