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

	struct Vertex_s
	{
		GLuint npoints;
		GLuint valuessize;
		GLfloat *values;
	} vertex = {0};
	vertex.valuessize += 60;
	vertex.values = calloc(vertex.valuessize, sizeof(*vertex.values));
	int ret = 0;
	do
	{
		char line[120] = {0};
		ret = fread(line, sizeof(char), 120, pFile);
		if (ret < 0)
		{
			err("glmotor: object file malformated %m");
			break;
		}
		char *end = strchr(line, '\n');
		if (end != NULL)
		{
			fseek(pFile, -ret, SEEK_CUR);
			fseek(pFile, end - line + 1, SEEK_CUR);
			*end = '\0';
		}
		if (! strncmp("v", line, 1))
		{
			vertex.npoints++;
			if ((vertex.valuessize / 3) < vertex.npoints)
			{
				vertex.valuessize += 60;
				vertex.values = realloc(vertex.values, vertex.valuessize * sizeof(*vertex.values));
			}
			int ret = sscanf(line + 1, "%f %f %f",
				&vertex.values[(vertex.npoints - 1) * 3 + 0],
				&vertex.values[(vertex.npoints - 1) * 3 + 1],
				&vertex.values[(vertex.npoints - 1) * 3 + 2]);
			if (ret != 3)
			{
				err("glmotor: object vertex malformated %d", ret);
				break;
			}
		}
		else if (! strncmp("vt", line, 2))
		{
		}
		else if (! strncmp("vn", line, 2))
		{
		}
		else if (! strncmp("f", line, 1))
		{
		}
	} while (ret != 0);

	GLMotor_Object_t *obj = object_create(motor, name, vertex.npoints, vertex.values);
	fclose(pFile);
	return obj;
}

GLMOTOR_EXPORT GLMotor_Texture_t * load_texture(GLMotor_t *motor, char *fileName)
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
	imagesize = attributes[4] / 8 * width * height;
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
	texture = texture_create(motor, width, height, buffer);
	free(buffer);
	return texture;
}
