#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "glmotor.h"
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
		err("glmotor: shader file '%s' opening error %m",fileName);
		return 0;
	}

	fileSize = getFileSize(pFile);

	if (fileContent == NULL)
		return fileSize;

	*fileContent = (char*)malloc(fileSize + 1);
	if ( *fileContent == NULL )
	{
		err("glmotor: shader file loading memory allocation error %m");
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

static void display_log(GLuint instance)
{
	GLint logSize = 0;
	GLchar* log = NULL;

	if (glIsProgram(instance))
		glGetProgramiv(instance, GL_INFO_LOG_LENGTH, &logSize);
	else
		glGetShaderiv(instance, GL_INFO_LOG_LENGTH, &logSize);
	log = (GLchar*)malloc(logSize);
	if ( log == NULL )
	{
		err("glmotor: Log memory allocation error %m");
		return;
	}
	if (glIsProgram(instance))
		glGetProgramInfoLog(instance, logSize, NULL, log);
	else
		glGetShaderInfoLog(instance, logSize, NULL, log);
	err("%s", log);
	free(log);
}

static void deleteProgram(GLuint programID, GLuint fragmentID, GLuint vertexID)
{
	if (programID)
	{
		glUseProgram(0);

		glDetachShader(programID, fragmentID);
		glDetachShader(programID, vertexID);

		glDeleteProgram(programID);
	}
	if (fragmentID)
		glDeleteShader(fragmentID);
	if (vertexID)
		glDeleteShader(vertexID);
}

static GLuint load_shader(GLenum type, const GLchar *sources[], GLuint size[], int nbsources)
{
	GLuint shaderID;

	shaderID = glCreateShader(type);
	if (shaderID == 0)
	{
		err("glmotor: shader creation error");
		return 0;
	}

	glShaderSource(shaderID, nbsources, sources, size);
	glCompileShader(shaderID);

	GLint compilationStatus = 0;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compilationStatus);
	if ( compilationStatus != GL_TRUE )
	{
		err("glmotor: shader compilation error");
		display_log(shaderID);
		glDeleteShader(shaderID);
		return 0;
	}
	return shaderID;
}

static GLuint program_build(const GLchar *vertexSource, GLuint vertexSize, const GLchar *fragmentSource[], GLuint fragmentSize[], int nbfragments)
{
	GLuint vertexID = load_shader(GL_VERTEX_SHADER, &vertexSource, &vertexSize, 1);
	if (vertexID == 0)
		return 0;
	GLuint fragmentID = load_shader(GL_FRAGMENT_SHADER, fragmentSource, fragmentSize, nbfragments);
	if (fragmentID == 0)
		return 0;

	GLuint programID = glCreateProgram();

	glAttachShader(programID, vertexID);
	glAttachShader(programID, fragmentID);

	glBindAttribLocation(programID, 0, "vPosition");

	glLinkProgram(programID);

	GLint programState = 0;
	glGetProgramiv(programID , GL_LINK_STATUS  , &programState);
	if ( programState != GL_TRUE)
	{
		display_log(programID);
		deleteProgram(programID, fragmentID, vertexID);
		return -1;
	}

	glDetachShader(programID, vertexID);
	glDetachShader(programID, fragmentID);

	glDeleteShader(vertexID);
	glDeleteShader(fragmentID);

	glUseProgram(programID);

	return programID;
}

GLMOTOR_EXPORT GLuint program_load(const char *vertex, const char *fragments[], int nbfragments)
{
	GLchar* vertexSource = NULL;
	GLuint vertexSize = 0;
	vertexSize = readFile(vertex, &vertexSource);

	GLchar* fragmentSource[MAX_FRAGS] = {0};
	GLuint fragmentSize[MAX_FRAGS] = {0};
	for (int i = 0; i < nbfragments; i++)
	{
		warn("glmotor: load %s", fragments[i]);
		fragmentSize[i] = readFile(fragments[i], &fragmentSource[i]);
		if (fragmentSize[i] == 0)
		{
			nbfragments--;
			break;
		}
	}

	if ( !vertexSource || !fragmentSource[0] )
	{
		return 0;
	}

	GLuint programID = program_build(vertexSource, vertexSize, fragmentSource, fragmentSize, nbfragments);
	for (int i = 0; i < nbfragments; i++)
	{
		free(fragmentSource[i]);
	}
	free(vertexSource);
	return programID;
}

GLMOTOR_EXPORT GLMotor_Object_t * object_load(GLMotor_t *motor, GLchar *name, const char *fileName, GLuint programID)
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
	object_setprogram(obj, programID);
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
		//dbg("obj: %.*s", end - line, line);
		char *next = NULL;
		numline++;
		if (line[0] == '#')
			continue;
		if (! strncmp("vc", line, 2))
		{
			next = line + 2;
			GLfloat color[4] = {0};
			int ret = sscanf(next, "%f %f %f %f",
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
			next = line + 2;
			GLfloat uv[3] = {0};
			int ret = sscanf(next, "%f %f", &uv[0], &uv[1]);
			if (ret != 2)
			{
				err("glmotor: object vertex misformated vt %d line %u", ret, numline);
				break;
			}
			object_appenduv(obj, 1, uv);
		}
		else if (!strncmp("vn", line, 2))
		{
			next = line + 2;
			GLfloat normal[3] = {0};
			int ret = sscanf(next, "%f %f %f", &normal[0], &normal[1], &normal[2]);
			if (ret != 3)
			{
				err("glmotor: object vertex misformated vn %d line %u", ret, numline);
				break;
			}
			object_appendnormal(obj, 1, normal);
		}
		else if (!strncmp("v", line, 1))
		{
			next = line + 1;
			GLfloat point[3] = {0};
			int ret = sscanf(next, "%f %f %f", &point[0], &point[1], &point[2]);
			if (ret != 3)
			{
				err("glmotor: object vertex misformated v %d line %u", ret, numline);
				break;
			}
			object_appendpoint(obj, 1, point);
		}
		else if (! strncmp("f", line, 1))
		{
			next = line + 1;
			GLuint position[3] = {0};
			GLuint uv[3] = {0};
			GLuint normal[3] = {0};
			int i = 0;
			char *s = NULL;
			while (i < 3)
			{
				while(isspace(*next)) next++;
				sscanf(next, "%u", &position[i]);
				s = strchr(next, '/');
				if ( s == NULL)
				{
					s = strchr(next, ' ');
					if ( s != NULL)
						next = s + 1;
					i++;
					continue;
				}
				next = s + 1;
				sscanf(next, "%u", &uv[i]);
				s = strchr(next, '/');
				if ( s == NULL)
				{
					i++;
					continue;
				}
				next = s + 1;
				sscanf(next, "%u", &normal[i]);
				s = strchr(next, ' ');
				if ( s == NULL)
				{
					i++;
					continue;
				}
				next = s + 1;
				i++;
			}
			object_appendface(obj, 1, position);
		}
		else if (! strncmp("usemtl", line, 6))
		{
			next = line + 6;
			while(isspace(*next)) next++;
			GLMotor_Texture_t *tex = texture_load(motor, next);
			if (tex)
				object_addtexture(obj, tex);
			else
				err("texture \"%s\" not supported", line + 7);
		}
		else if (! strncmp("ki", line, 2))
		{
			next = line + 2;
			while(isspace(*next)) next++;
			GLuint format = 1;
			GLMotor_Translate_t tr = {0};
			GLMotor_Translate_t *ptr = &tr;
			GLMotor_Rotate_t rotate = { 0 };
			GLMotor_Rotate_t *protate = &rotate;
			int repeats = 1;
			int ret = 1;
			while (ret > 0 && next)
			{
				switch (format)
				{
				case 1:
					ret = sscanf(next, "%f", &tr.coord.L);
					if (ret == 0)
						tr.coord.L = 1.0;
					format = 2;
					ret = 1;
				break;
				case 2:
					next = strchr(next, '(');
					if (next == NULL) break;
					next++;
					while(isspace(*next)) next++;
					ret = sscanf(next, "%f %f %f",
						&tr.coord.X, &tr.coord.Y, &tr.coord.Z);
					if (ret == 0)
						ptr = NULL;
					format = 3;
					ret = 1;
				break;
				case 3:
					next = strchr(next, ')');
					if (next == NULL) break;
					next++;
					while(isspace(*next)) next++;
					ret = sscanf(next, "%f",
						&rotate.ra.A);
					if (ret == 0)
						protate = NULL;
					format = 4;
					ret = 1;
				break;
				case 4:
					next = strchr(next, '(');
					if (next == NULL) break;
					next++;
					while(isspace(*next)) next++;
					ret = sscanf(next, "%f %f %f",
						&rotate.ra.X, &rotate.ra.Y, &rotate.ra.Z);
					if (ret == 0)
						protate = NULL;
					format = 5;
					ret = 1;
				break;
				case 5:
					next = strchr(next, ')');
					if (next == NULL) break;
					next++;
					while(isspace(*next)) next++;
					ret = sscanf(next, "%d", &repeats);
					if (ret == 0)
						ret = -1;
				break;
				}
			}
			if (ptr || protate)
				object_appendkinematic(obj, ptr, protate, repeats);
		}
		else
		{
			warn("glmotor: obj file entry not supported\n\t%.*s", end - line, line);
		}
	} while (ret != 0);


	fclose(pFile);
	return obj;
}

GLMOTOR_EXPORT GLMotor_Texture_t * texture_loadTGA(GLMotor_t *motor, const char *fileName)
{
	char *buffer = NULL;
	FILE *f = NULL;
	unsigned char tgaheader[12];
	unsigned char attributes[6];
	unsigned int imagesize;

	f = fopen(fileName, "rb");
	if(f == NULL)
	{
		return NULL;
	}

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
		fclose(f);
		return NULL;
	}
	fclose(f);

	GLMotor_Texture_t *texture = NULL;
	texture = texture_create(motor, width, height, FOURCC('A','B','2','4'), 1, buffer);
	free(buffer);
	return texture;
}

GLMOTOR_EXPORT GLMotor_Texture_t * texture_loadDDS(GLMotor_t *motor, const char *fileName)
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

GLMOTOR_EXPORT GLMotor_Texture_t * texture_loadV4L2(GLMotor_t *motor, const char *config)
{
	const char *next = config;
	char device[255] = "/dev/video0";
	GLuint width = 640;
	GLuint height = 480;
	uint32_t fourcc = FOURCC('Y','U','Y','V');
	char fcc1 = 0, fcc2 = 0, fcc3 = 0, fcc4 = 0;
	next = strchr(next, '(');
	if (next)
	{
		next++;
		sscanf(next, "\"%s\"", device);
		char *end = strchr(device, '\"');
		if (end)
			end[0] = '\0';
		next = strchr(next + 1, ',');
		if (next && next[1] == ' ') next++;
	}
	if (next)
	{
		next++;
		sscanf(next, "%u", &width);
		next = strchr(next, ',');
		if (next && next[1] == ' ') next++;
	}
	if (next)
	{
		next++;
		sscanf(next, "%u", &height);
		next = strchr(next, ',');
		if (next && next[1] == ' ') next++;
	}
	if (next)
	{
		next++;
		sscanf(next, "%c", &fcc1);
	}
	if (next)
	{
		next++;
		sscanf(next, "%c", &fcc2);
	}
	if (next)
	{
		next++;
		sscanf(next, "%c", &fcc3);
	}
	if (next)
	{
		next++;
		sscanf(next, "%c", &fcc4);
		next = strchr(next + 1, ')');
	}
	if (next)
	{
		fourcc = FOURCC(fcc1, fcc2, fcc3, fcc4);
	}
	GLMotor_Texture_t *tex = texture_fromcamera(motor, device, width, height, fourcc);
	return tex;
}

GLMOTOR_EXPORT GLMotor_Texture_t * texture_loaddefault(GLMotor_t *motor, const char *config)
{
	GLubyte pixels[4 * 4] =
	{
		255,   0,   0, // Red
		  0, 255,   0, // Green
		  0,   0, 255, // Blue
		255, 255,   0  // Yellow
	};
	GLMotor_Texture_t *tex = texture_create(motor, 2, 2, FOURCC('R', 'G', '2', '4'), 0, pixels);
	return tex;
}

GLMOTOR_EXPORT GLMotor_Texture_t * texture_load(GLMotor_t *motor, const char *config)
{
	size_t length = strlen(config);
	if (length >= 6 && !strncmp("default", config, 6))
		return texture_loaddefault(motor, config + 6);
	if (length >= 4 &&!strncmp("v4l2", config, 4))
		return texture_loadV4L2(motor, config + 4);
	if (!strncmp(config + length - 4, ".tga",4))
		return texture_loadTGA(motor, config);
	if (!strncmp(config + length - 4, ".dds",4))
		return texture_loadDDS(motor, config);
	return NULL;
}
