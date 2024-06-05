#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

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

GLMOTOR_EXPORT GLuint load_shader(GLMotor_t *motor, const char *vertex, const char *fragment)
{
	GLchar* vertexSource = NULL;
	GLchar* fragmentSource = NULL;

	GLuint vertexSize = readFile(vertex, &vertexSource);
	GLuint fragmentSize = readFile(fragment, &fragmentSource);

	if ( !vertexSource || !fragmentSource )
	{
		return 0;
	}

	GLuint programID = build_program(motor, vertexSource, vertexSize, fragmentSource, fragmentSize);
	free(vertexSource);
	free(fragmentSource);
	return programID;
}

GLMOTOR_EXPORT GLMotor_Object_t * load_obj(GLMotor_t *motor, const char *fileName)
{
	FILE* pFile = NULL;
	GLint fileSize = 0;

	pFile = fopen(fileName, "r");
	if ( pFile == NULL )
	{
		err("segl: shader file '%s' opening error %m",fileName);
		return 0;
	}

	struct Vertex_s
	{
		GLuint valuessize;
		GLuint npoints;
		GLfloat *values;
	} vertex = {0};
	vertex.valuessize += 50;
	vertex.values = calloc(vertex.valuessize * 3, sizeof(*vertex.values));
	while (1)
	{
		char line[120];
		int ret = fscanf(pFile, "%120s", line);
		if (ret != 1)
			break;
		if (! strcmp("v", line))
		{
			vertex.npoints++;
			if (vertex.valuessize < vertex.npoints)
			{
				vertex.valuessize += 50;
				vertex.values = realloc(vertex.values, vertex.valuessize * 3 * sizeof(*vertex.values));
			}
			ret = fscanf(pFile, "%f %f %f",
				&vertex.values[(vertex.npoints - 1) * 3 + 0],
				&vertex.values[(vertex.npoints - 1) * 3 + 1],
				&vertex.values[(vertex.npoints - 1) * 3 + 2]);
			if (ret != 3)
				break;
		}
		else if (! strcmp("vt", line))
		{
		}
		else if (! strcmp("vn", line))
		{
		}
		else if (! strcmp("f", line))
		{
		}
	}
	GLMotor_Object_t *obj = create_object(motor, vertex.npoints, vertex.values);
	free(vertex.values);
	fclose(pFile);
	return obj;
}
