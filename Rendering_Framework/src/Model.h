#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <unordered_map>
#include <assert.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cmath>
#include <iostream>
#include <string>
#include <algorithm>

class Model {
public:
    GLuint vao;
    struct Mesh {
        unsigned int count;
        unsigned int verticesOffset;
        unsigned int indicesOffset;
    };
    struct Texture {
        GLuint textureID;
    };
    std::vector<Mesh> meshes;
    std::vector<unsigned char> textures;
	GLuint textureID;
private:
	const int NUM_TEXTURE = 3;
	const int IMG_WIDTH = 1024;
	const int IMG_HEIGHT = 1024;
	const int IMG_CHANNEL = 4;
	std::vector<float> vertices;
	std::vector<int> indices;
    Mesh processMesh(const aiMesh *mesh) {
        // load data into vertex buffers
        unsigned int verticesOffset = vertices.size();
		unsigned int indicesOffset = indices.size();

        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D vert = mesh->mVertices[i];
            aiVector3D norm = mesh->mNormals[i];
            aiVector3D uv = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][i] : aiVector3D(0.0);

            //verts
            vertices.push_back(vert.x);
            vertices.push_back(vert.y);
            vertices.push_back(vert.z);

            //normals
            vertices.push_back(norm.x);
            vertices.push_back(norm.y);
            vertices.push_back(norm.z);

            //uvs
            vertices.push_back(uv.x);
            vertices.push_back(uv.y);
            vertices.push_back(meshes.size());
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            assert(face.mNumIndices == 3);
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
		assert(mesh->mNumVertices * 9 == vertices.size() - verticesOffset);
		assert(mesh->mNumFaces * 3 == indices.size() - indicesOffset);

        std::cout << "Mesh loaded: " << mesh->mName.C_Str() << " material id: " << mesh->mMaterialIndex << std::endl;
		std::cout << "Count: " << 3 * mesh->mNumFaces << std::endl;
		std::cout << "Vertices offest: " << verticesOffset << std::endl;
		std::cout << "Indices offest: " << indicesOffset << std::endl;

        return Mesh{ 3 * mesh->mNumFaces, verticesOffset/9, indicesOffset/3 };
    }

public:
	Model() {}

    void loadModel(std::string const &pFile){
		std::cout << "========================" << std::endl;
		std::cout << "Loading model: " << pFile << std::endl;

        Assimp::Importer importer;

        const aiScene* scene = importer.ReadFile(pFile.c_str(),
            aiProcess_Triangulate |
            aiProcess_GenNormals |
            aiProcess_FlipUVs
        );

        // If the import failed, report it
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            //DoTheErrorLogging(aiGetErrorString());
            std::cout << "ERROR ASSIMP " << importer.GetErrorString() << std::endl;
        }

        // Now we can access the file's contents
        std::cout << "Meshes count: " << scene->mNumMeshes << std::endl;

        aiMesh *mesh = scene->mMeshes[scene->mRootNode->mChildren[0]->mMeshes[0]];
        this->meshes.push_back(processMesh(mesh));
    }

	void buildVAO() {
		GLuint vbo, ebo;
		// create buffers/arrays
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenBuffers(1, &ebo);

		glBindVertexArray(vao);

		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);

		// set the vertex attribute pointers
		// vertex Positions
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (GLvoid*)0);
		// vertex normals
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (GLvoid*)(sizeof(float) * 3));
		// vertex texture coords
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (GLvoid*)(sizeof(float) * 6));

		glBindVertexArray(0);
		std::cout << "VAO: " << vao << std::endl;
	}

    void loadTexture(std::string const &textureFile) {

        int width, height, nrComponents;
        unsigned char *data = stbi_load(textureFile.c_str(), &width, &height, &nrComponents, 4);
		std::cout << width << " " << height << " " << nrComponents << " " << std::endl;
        if (data)
        {
			textures.insert(textures.end(), data, data + (1024 * 1024 * 4) * sizeof(unsigned char));

            std::cout << "Texture loaded: " << textureFile << std::endl;
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << textureFile << std::endl;
            stbi_image_free(data);
        }
    }

	void buildTexture() {
		glGenTextures(1, &textureID);
		// create buffers/arrays
		glActiveTexture(GL_TEXTURE0 + textureID);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureID);
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA32F, IMG_WIDTH, IMG_HEIGHT, NUM_TEXTURE);
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, IMG_WIDTH, IMG_HEIGHT, NUM_TEXTURE, GL_RGBA, GL_UNSIGNED_BYTE, textures.data());
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

		std::cout << "Texture: " << textureID << std::endl;
	}
};