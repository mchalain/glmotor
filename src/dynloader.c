#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <GL/gl.h>

#include "loader.h"
#include "log.h"

static GLint getFileSize(FILE* const pFile)
{
	GLint length = 0;

	fseek(pFile,0,SEEK_END);
	length = ftell(pFile);
	fseek(pFile,0,SEEK_SET);

	return length;
}

static GLint readFile(const char* fileName, char** fileContent)
{
	FILE* pFile = NULL;
	GLint fileSize = 0;

	pFile = fopen(fileName, "r");
	if ( pFile == NULL )
	{
		err("segl: shader file '%s' opening error %m",fileName);
		return 0;
	}

	fileSize = getFileSize(pFile);

	*fileContent = (char*)malloc(fileSize + 1);
	if ( *fileContent == NULL )
	{
		err("segl: shader file loading memory allocation error %m");
		return 0;
	}

	if (fread(*fileContent, fileSize, 1, pFile) < 0)
	{
		fclose(pFile);
		err("glmotor: File loading error %m");
	}
	(*fileContent)[fileSize] = '\0';

	fclose(pFile);

	return fileSize;
}

GLMOTOR_EXPORT GLuint glmotor_load(GLMotor_t *motor, const char *vertex, const char *fragment)
{
	GLchar* vertexSource = NULL;
	GLchar* fragmentSource = NULL;

	GLuint vertexSize = readFile(vertex, &vertexSource);
	GLuint fragmentSize = readFile(fragment, &fragmentSource);

	if ( !vertexSource || !fragmentSource )
	{
		return 0;
	}

	GLuint programID = glmotor_build(motor, vertexSource, vertexSize, fragmentSource, fragmentSize);
	free(vertexSource);
	free(fragmentSource);
	return programID;
}

GLMOTOR_EXPORT GLMotor_Object_t * object_load(GLMotor_t *motor, GLchar *name, const char *fileName)
{
	FILE* pFile = NULL;
	GLint fileSize = 0;

	pFile = fopen(fileName, "r");
	if ( pFile == NULL )
	{
		err("glmotor: shader file '%s' opening error %m",fileName);
		return 0;
	}

	GLuint npoints = 0;
	GLuint nfaces = 0;
	GLuint ntextures = 0;
	GLuint nnormals = 0;
	GLuint ncolors = 0;
	int ret = 0;
	do
	{
		char line[120] = {0};
		ret = fread(line, sizeof(char), 120, pFile);
		if (ret < 0)
		{
			err("glmotor: object file misformated %m");
			break;
		}
		char *end = strchr(line, '\n');
		if (end != NULL)
		{
			fseek(pFile, -ret, SEEK_CUR);
			fseek(pFile, end - line + 1, SEEK_CUR);
			*end = '\0';
		}
		if (! strncmp("vc", line, 2))
		{
			ncolors++;
		}
		else if (! strncmp("vt", line, 2))
		{
			ntextures++;
		}
		else if (! strncmp("vn", line, 2))
		{
			nnormals++;
		}
		else if (! strncmp("v", line, 1))
		{
			npoints++;
		}
		else if (! strncmp("f", line, 1))
		{
			nfaces++;
		}
	} while (ret != 0);
	fseek(pFile, 0, SEEK_SET);

	if ((ncolors && ncolors != npoints) ||
		(ntextures && ntextures != npoints) ||
		(nnormals && nnormals != npoints))
		err("glmotor: object file misformated not enought colors, textures or normals");
	GLMotor_Object_t *obj = object_create(motor, name, npoints, nfaces);
	GLuint numline = 0;
	do
	{
		char line[120] = {0};
		ret = fread(line, sizeof(char), 120, pFile);
		if (ret < 0)
		{
			err("glmotor: object file misformated %m");
			break;
		}
		char *end = strchr(line, '\n');
		if (end != NULL)
		{
			fseek(pFile, -ret, SEEK_CUR);
			fseek(pFile, end - line + 1, SEEK_CUR);
			*end = '\0';
		}
		numline++;
		if (! strncmp("vc", line, 2))
		{
			GLfloat color[4] = {0};
			int ret = sscanf(line + 2, "%f %f %f %f",
				&color[0], &color[1], &color[2], &color[3]);
			if (ret == 3)
			{
				color[3] = 1.0f;
				ret = 4;
			}
			if (ret != 4)
			{
				err("glmotor: object vertex misformated vc %d line %u", ret, numline);
				break;
			}
			object_appendcolor(obj, 1, color);
		}
		else if (! strncmp("vt", line, 2))
		{
			GLfloat uv[3] = {0};
			int ret = sscanf(line + 2, "%f %f",
				&uv[0], &uv[1]);
			if (ret != 2)
			{
				err("glmotor: object vertex misformated vt %d line %u", ret, numline);
				break;
			}
			object_appenduv(obj, 1, uv);
		}
		else if (! strncmp("vn", line, 2))
		{
			GLfloat normal[3] = {0};
			int ret = sscanf(line + 2, "%f %f %f",
				&normal[0], &normal[1], &normal[2]);
			if (ret != 3)
			{
				err("glmotor: object vertex misformated vn %d line %u", ret, numline);
				break;
			}
			object_appendnormal(obj, 1, normal);
		}
		else if (! strncmp("v", line, 1))
		{
			GLfloat point[3] = {0};
			int ret = sscanf(line + 1, "%f %f %f",
				&point[0], &point[1], &point[2]);
			if (ret != 3)
			{
				err("glmotor: object vertex misformated v %d line %u", ret, numline);
				break;
			}
			object_appendpoint(obj, 1, point);
		}
		else if (! strncmp("f", line, 1))
		{
			GLuint position[3] = {0};
			GLuint uv[3] = {0};
			GLuint normal[3] = {0};
			GLuint format = 1;
			int ret = 0;
			while (ret == 0)
			{
				switch (format)
				{
				case 1:
					ret = sscanf(line + 1, "%u %u %u",
						&position[0], &position[1], &position[2]);
				break;
				case 2:
					ret = sscanf(line + 1, "%u/%u %u/%u %u/%u",
						&position[0], &uv[0],
						&position[1], &uv[1],
						&position[2], &uv[2]);
				break;
				case 3:
					ret = sscanf(line + 1, "%u/%u/%u %u/%u/%u %u/%u/%u",
						&position[0], &uv[0], &normal[0],
						&position[1], &uv[1], &normal[1],
						&position[2], &uv[2], &normal[2]);
				break;
				default:
					ret = -1;
				}
				if (ret != 3 * format)
				{
					format++;
					ret = 0;
				}
			}
			if (ret != 3 * format)
			{
				err("glmotor: object vertex misformated f %d line %u", ret, numline);
				break;
			}
			object_appendface(obj, 1, position);
		}
		else if (! strncmp("mat", line, 1))
		{
			int ret = 0;
			GLMotor_Texture_t *tex = NULL;
			if (!strncmp(" v4l2", line + 3, 5))
			{
				char device[255] = "/dev/video0";
				GLuint width = 640;
				GLuint height = 480;
				uint32_t fourcc = FOURCC('Y','U','Y','V');
				if (line[3 + 5] == '(')
				{
					char fcc1 = 0, fcc2 = 0, fcc3 = 0, fcc4 = 0;
					char *next = NULL;
					next = strchr(line + 3 + 5, '(');
					if (next)
					{
						sscanf(next + 1, "\"%s\"", device);
						char *end = strchr(device, '\"');
						if (end)
							end[0] = '\0';
						next = strchr(next + 1, ',');
						if (next[1] == ' ') next++;
					}
					if (next)
					{
						sscanf(next + 1, "%u", &width);
						next = strchr(next + 1, ',');
						if (next[1] == ' ') next++;
					}
					if (next)
					{
						sscanf(next + 1, "%u", &height);
						next = strchr(next + 1, ',');
						if (next[1] == ' ') next++;
					}
					if (next)
					{
						sscanf(next + 1, "%c", &fcc1);
						next++;
					}
					if (next)
					{
						sscanf(next + 1, "%c", &fcc2);
						next++;
					}
					if (next)
					{
						sscanf(next + 1, "%c", &fcc3);
						next++;
					}
					if (next)
					{
						sscanf(next + 1, "%c", &fcc4);
						next++;
					}
					fourcc = FOURCC(fcc1, fcc2, fcc3, fcc4);
				}
				tex = texture_fromcamera(motor, device, width, height, fourcc);
			}
			else if (!strncmp(" default", line + 3, 7))
			{
				GLubyte pixels[4 * 4] =
				{
					255,   0,   0, 255,// Red
					  0, 255,   0, 255, // Green
					  0,   0, 255, 255, // Blue
					255, 255,   0, 255  // Yellow
				};
				tex = texture_create(motor, 2, 2, FOURCC('A', 'B', '2', '4'), 1, pixels);
			}
			else
			{
				tex = texture_load(motor, line + 4);
			}
			if (tex)
				object_addtexture(obj, tex);
		}
	} while (ret != 0);
	

	fclose(pFile);
	return obj;
}

GLMOTOR_EXPORT GLMotor_Texture_t * texture_loadTGA(GLMotor_t *motor, char *fileName)
{
	char *buffer = NULL;
	FILE *f;
	unsigned char tgaheader[12];
	unsigned char attributes[6];
	unsigned int imagesize;

	f = fopen(fileName, "rb");
	if(f == NULL) return NULL;

	if(fread(&tgaheader, sizeof(tgaheader), 1, f) == 0)
	{
		fclose(f);
		return NULL;
	}

	if(fread(attributes, sizeof(attributes), 1, f) == 0)
	{
		fclose(f);
		return 0;
	}

	GLuint width = 0;
	width = attributes[1] * 256 + attributes[0];
	GLuint height = 0;
	height = attributes[3] * 256 + attributes[2];
	GLuint depth = attributes[4];
	imagesize = depth / 8 * width * height;
	buffer = malloc(imagesize);
	if (buffer == NULL)
	{
		fclose(f);
		return 0;
	}

	if(fread(buffer, 1, imagesize, f) != imagesize)
	{
		free(buffer);
		return NULL;
	}
	fclose(f);

	GLMotor_Texture_t *texture = NULL;
	texture = texture_create(motor, width, height, FOURCC('A','B','2','4'), 1, buffer);
	free(buffer);
	return texture;
}

GLMOTOR_EXPORT GLMotor_Texture_t * texture_loadDDS(GLMotor_t *motor, char *fileName)
{
	unsigned char header[124];

	FILE *fp;

	/* try to open the file */
	fp = fopen(fileName, "rb");
	if (fp == NULL)
	{
		err("%s could not be opened. Are you in the right directory", fileName);
		return 0;
	}

	/* verify the type of file */
	char filecode[4];
	fread(filecode, 1, 4, fp);
	if (strncmp(filecode, "DDS ", 4) != 0)
	{
		fclose(fp);
		return 0;
	}

	/* get the surface desc */
	fread(&header, 124, 1, fp);

	unsigned int height      = *(unsigned int*)&(header[8 ]);
	unsigned int width	     = *(unsigned int*)&(header[12]);
	unsigned int linearSize	 = *(unsigned int*)&(header[16]);
	unsigned int mipMapCount = *(unsigned int*)&(header[24]);
	unsigned int fourCC      = *(unsigned int*)&(header[80]);

	unsigned char * buffer;
	unsigned int bufsize;
	/* how big is it going to be including all mipmaps? */
	bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
	buffer = (unsigned char*)malloc(bufsize * sizeof(unsigned char));
	fread(buffer, 1, bufsize, fp);
	/* close the file pointer */
	fclose(fp);

	GLMotor_Texture_t *tex = texture_create(motor, width, height, fourCC, mipMapCount, buffer);

	free(buffer);

	return tex;
}

GLMOTOR_EXPORT GLMotor_Texture_t * texture_load(GLMotor_t *motor, char *fileName)
{
	size_t length = strlen(fileName);
	if (!strncmp(fileName + length - 4, ".tga",4))
		return texture_loadTGA(motor, fileName);
	if (!strncmp(fileName + length - 4, ".dds",4))
		return texture_loadDDS(motor, fileName);
}
