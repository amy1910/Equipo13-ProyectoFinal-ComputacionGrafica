#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "SOIL2/SOIL2.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include  "Shader.h"
#include "PathUtils.h"

using namespace std;

inline GLint CreateSolidTexture(const glm::vec3& color);
inline GLint TextureFromFile(const char *path, string directory,
	const glm::vec3& fallbackColor = glm::vec3(1.0f));

class Model
{
public:
	/*  Functions   */
	// Constructor, expects a filepath to a 3D model.
	Model(const GLchar *path)
	{
		this->loadModel(ResolveAssetPath(path));
	}

	// Replaces the diffuse map for every mesh in the model. This keeps the
	// material choice explicit and provides a reliable fallback when an OBJ/MTL
	// exporter omitted its diffuse reference.
	void OverrideDiffuseTexture(const char *filePath)
	{
		const std::string resolvedPath = ResolveAssetPath(filePath);
		GLint textureID = TextureFromFile(resolvedPath.c_str(), "", glm::vec3(1.0f));

		for (Mesh &mesh : this->meshes)
		{
			vector<Texture> nonDiffuseTextures;
			for (const Texture &texture : mesh.textures)
			{
				if (texture.type != "texture_diffuse")
				{
					nonDiffuseTextures.push_back(texture);
				}
			}

			Texture diffuseTexture;
			diffuseTexture.id = textureID;
			diffuseTexture.type = "texture_diffuse";
			diffuseTexture.path = resolvedPath.c_str();
			nonDiffuseTextures.push_back(diffuseTexture);
			mesh.textures = nonDiffuseTextures;
		}
	}

	void SetEmissive(const glm::vec3& color, float strength)
	{
		emissiveColor_ = color;
		emissiveStrength_ = strength;
	}

	void ApplyMaterialUniforms(Shader& shader) const
	{
		glUniform3f(glGetUniformLocation(shader.Program, "material.emissiveColor"),
			emissiveColor_.r, emissiveColor_.g, emissiveColor_.b);
		glUniform1f(glGetUniformLocation(shader.Program, "material.emissiveStrength"),
			emissiveStrength_);
	}

	// Draws the model, and thus all its meshes
	void Draw(Shader shader)
	{
		for (GLuint i = 0; i < this->meshes.size(); i++)
		{
			this->meshes[i].Draw(shader);
		}
	}

private:
	glm::vec3 emissiveColor_{0.0f};
	float emissiveStrength_ = 0.0f;
	/*  Model Data  */
	vector<Mesh> meshes;
	string directory;
	vector<Texture> textures_loaded;	// Stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.

										/*  Functions   */
										// Loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
	void loadModel(string path)
	{
		// Read file via ASSIMP
		Assimp::Importer importer;
		const aiScene *scene = importer.ReadFile(path,
			aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals);

		// Check for errors
		if (!scene || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}
		// Retrieve the directory path of the filepath
		const size_t directorySlash = path.find_last_of("/\\");
		this->directory = directorySlash == string::npos
			? string() : path.substr(0, directorySlash);

		// Process ASSIMP's root node recursively
		this->processNode(scene->mRootNode, scene);
	}

	// Processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
	void processNode(aiNode* node, const aiScene* scene)
	{
		// Process each mesh located at the current node
		for (GLuint i = 0; i < node->mNumMeshes; i++)
		{
			// The node object only contains indices to index the actual objects in the scene.
			// The scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			this->meshes.push_back(this->processMesh(mesh, scene));
		}

		// After we've processed all of the meshes (if any) we then recursively process each of the children nodes
		for (GLuint i = 0; i < node->mNumChildren; i++)
		{
			this->processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh *mesh, const aiScene *scene)
	{
		// Data to fill
		vector<Vertex> vertices;
		vector<GLuint> indices;
		vector<Texture> textures;

		// Walk through each of the mesh's vertices
		for (GLuint i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector; // We declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.

							  // Positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;

			// Normals. Some valid OBJ parts omit vn records; Assimp normally
			// generates them through aiProcess_GenSmoothNormals, but keep this
			// guard so a malformed or intentionally unlit mesh cannot crash the
			// loader with an out-of-bounds read.
			if (mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			else
			{
				vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
			}

			// Texture Coordinates
			if (mesh->mTextureCoords[0]) // Does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// A vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
			{
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);
			}

			vertices.push_back(vertex);
		}

		// Now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (GLuint i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// Retrieve all indices of the face and store them in the indices vector
			for (GLuint j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}

		// Process materials
		glm::vec3 materialColor(1.0f);
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
			aiColor3D diffuseColor;
			if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == AI_SUCCESS)
			{
				materialColor = glm::vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
			}
			// We assume a convention for sampler names in the shaders. Each diffuse texture should be named
			// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
			// Same applies to other texture as the following list summarizes:
			// Diffuse: texture_diffuseN
			// Specular: texture_specularN
			// Normal: texture_normalN

			// 1. Diffuse maps
			vector<Texture> diffuseMaps = this->loadMaterialTextures(
				material, aiTextureType_DIFFUSE, "texture_diffuse", materialColor);
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

			// 2. Specular maps
			vector<Texture> specularMaps = this->loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}

		// Return a mesh object created from the extracted mesh data
		return Mesh(vertices, indices, textures, materialColor);
	}

	// Checks all material textures of a given type and loads the textures if they're not loaded yet.
	// The required info is returned as a Texture struct.
	vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type,
		string typeName, const glm::vec3& fallbackColor = glm::vec3(1.0f))
	{
		vector<Texture> textures;

		for (GLuint i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);

			// Check if texture was loaded before and if so, continue to next iteration: skip loading a new texture
			GLboolean skip = false;

			for (GLuint j = 0; j < textures_loaded.size(); j++)
			{
				if (textures_loaded[j].path == str)
				{
					textures.push_back(textures_loaded[j]);
					skip = true; // A texture with the same filepath has already been loaded, continue to next one. (optimization)

					break;
				}
			}

			if (!skip)
			{   // If texture hasn't been loaded already, load it
				Texture texture;
				texture.id = TextureFromFile(str.C_Str(), this->directory, fallbackColor);
				texture.type = typeName;
				texture.path = str;
				textures.push_back(texture);

				this->textures_loaded.push_back(texture);  // Store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			}
		}

		return textures;
	}
};

inline GLint CreateSolidTexture(const glm::vec3& color)
{
	const auto toByte = [](float value) -> unsigned char {
		return static_cast<unsigned char>(std::round(std::max(0.0f, std::min(1.0f, value)) * 255.0f));
	};
	const unsigned char pixel[4]{toByte(color.r), toByte(color.g), toByte(color.b), 255};

	GLuint textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	return static_cast<GLint>(textureID);
}

inline GLint TextureFromFile(const char *path, string directory, const glm::vec3& fallbackColor)
{
	const string rawPath = path ? string(path) : string();
	const size_t slash = rawPath.find_last_of("/\\");
	const string basename = slash == string::npos ? rawPath : rawPath.substr(slash + 1);
	const bool absolutePath = rawPath.size() > 2 &&
		((rawPath[1] == ':' && (rawPath[2] == '\\' || rawPath[2] == '/'))
			|| rawPath[0] == '/' || rawPath[0] == '\\');
	// Keep relative MTL paths such as ../mariachi_atlas.png intact. The old
	// basename-only join incorrectly searched inside each character folder.
	// Absolute exporter paths are reduced to their basename so an external
	// directory can never become a runtime dependency.
	string filename;
	if (directory.empty())
	{
		filename = rawPath;
	}
	else
	{
		filename = directory + '/' + (absolutePath ? basename : rawPath);
	}
	int width = 0;
	int height = 0;
	unsigned char *image = SOIL_load_image(filename.c_str(), &width, &height, 0, SOIL_LOAD_RGBA);
	if (image == nullptr || width <= 0 || height <= 0)
	{
		if (image != nullptr)
		{
			SOIL_free_image_data(image);
		}
		// A material must remain visible even when an exporter omitted map_Kd or
		// a copied image is unavailable. Use its MTL Kd color rather than black.
		return CreateSolidTexture(fallbackColor);
	}

	GLuint textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
	GLfloat maxAnisotropy = 1.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
		std::min(maxAnisotropy, 8.0f));
#endif
	glBindTexture(GL_TEXTURE_2D, 0);
	SOIL_free_image_data(image);

	return textureID;
}
