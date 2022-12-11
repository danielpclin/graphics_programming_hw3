#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <unordered_map>
#include <assert.h>
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#include <cmath>
#include <iostream>
#include <string>
#include <algorithm>

class ModelOld {
public:
	struct Mesh {
		GLuint vao;
		unsigned int indicesCount;
		unsigned int materialID;
	};
	struct Texture {
		GLuint textureID;
	};
	//std::vector<Mesh> meshes;
	//std::unordered_map<int, Texture> diffuse;
	Mesh mesh;
	Texture texture;
private:

	Mesh processMesh(const aiMesh *mesh, const aiScene *scene) {
		GLuint vao, vbo, ebo;
		// create buffers/arrays
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);

		// load data into vertex buffers
		int vertices_size = sizeof(float) * 8 * mesh->mNumVertices;
		float *vertices = (float*)malloc(vertices_size);

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			aiVector3D vert = mesh->mVertices[i];
			aiVector3D norm = mesh->mNormals[i];
			aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0);

			//verts
			vertices[i * 8] = vert.x;
			vertices[i * 8 + 1] = vert.y;
			vertices[i * 8 + 2] = vert.z;

			//normals
			vertices[i * 8 + 3] = norm.x;
			vertices[i * 8 + 4] = norm.y;
			vertices[i * 8 + 5] = norm.z;

			//uvs
			vertices[i * 8 + 6] = uv.x;
			vertices[i * 8 + 7] = uv.y;
		}

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices_size, vertices, GL_STATIC_DRAW);
		free(vertices);

		int indices_size = sizeof(int) * 3 * mesh->mNumFaces;
		unsigned int *indices = (unsigned int*)malloc(indices_size);

		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			assert(face.mNumIndices == 3);
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices[i * 3 + j] = face.mIndices[j];
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size, indices, GL_STATIC_DRAW);
		free(indices);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 3));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 8, (GLvoid*)(sizeof(float) * 6));

		glBindVertexArray(0);
		std::cout << "Mesh loaded: " << mesh->mName.C_Str() << " material id: " << mesh->mMaterialIndex << std::endl;
		return Mesh{ vao, 3 * mesh->mNumFaces, mesh->mMaterialIndex };
	}
	void processNode(aiNode *node, const aiScene *scene) {
		aiMesh *mesh = scene->mMeshes[node->mChildren[0]->mMeshes[0]];
		this->mesh = processMesh(mesh, scene);
	}
	Texture loadTexture(std::string const &pFile) {
		GLuint textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char *data = stbi_load(pFile.c_str(), &width, &height, &nrComponents, 4);
		if (data)
		{
			glActiveTexture(GL_TEXTURE0 + textureID);
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			std::cout << "Texture loaded: " << pFile << std::endl;
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load at path: " << pFile << std::endl;
			stbi_image_free(data);
		}
		return Texture{ textureID };
	}

public:

	ModelOld(std::string const &pFile, std::string const &textureFile) {
		Assimp::Importer importer;

		const aiScene* scene = importer.ReadFile(pFile.c_str(),
			aiProcess_Triangulate |
			aiProcess_JoinIdenticalVertices |
			aiProcess_GenNormals |
			aiProcess_FlipUVs |
			aiProcess_GenUVCoords |
			aiProcess_SortByPType
		);

		// If the import failed, report it
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			//DoTheErrorLogging(aiGetErrorString());
			std::cout << "ERROR ASSIMP " << importer.GetErrorString() << std::endl;
		}

		// Now we can access the file's contents
		std::cout << "Meshes count: " << scene->mNumMeshes << std::endl;
		processNode(scene->mRootNode, scene);
		this->texture = loadTexture(textureFile);
	}
};