///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// Initialize the texture collection.
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// Cleans up and deallocates any loaded OpenGL textures before destruction.
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

void SceneManager::LoadSceneTextures()
{
	// Load Mug_Texture.jpg for the mug cylinder and torus.
	if (CreateGLTexture("Textures/Mug_Texture.jpg", "mugTexture"))
	{
		std::cout << "Loaded mugTexture (Mug_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load mugTexture (Mug_Texture.jpg)!\n";
	}

	// Load Table_Texture.jpg into memory for the plane.
	if (CreateGLTexture("Textures/Table_Texture.jpg", "tableTexture"))
	{
		std::cout << "Loaded tableTexture (Table_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load tableTexture (Table_Texture.jpg)!\n";
	}

	// Load Trout_Texture.jpg into memory for the fish.
	if (CreateGLTexture("Textures/Trout_Texture.jpg", "troutTexture"))
	{
		std::cout << "Loaded troutTexture (Trout_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load troutTexture (Trout_Texture.jpg)!\n";
	}

	// Load Rod_Texture.jpg into memory for the fishing rod.
	if (CreateGLTexture("Textures/Rod_Texture.jpg", "rodTexture"))
	{
		std::cout << "Loaded rodTexture (Rod_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load rodTexture (Rod_Texture.jpg)!\n";
	}
	
	// Load Box_Texture.jpg into memory for the tackle box.
	if (CreateGLTexture("Textures/Box_Texture.jpg", "boxTexture"))
	{
		std::cout << "Loaded boxTexture (Box_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load boxTexture (Box_Texture.jpg)!\n";
	}
	
	// Load Cork_Texture.jpg into memory for the fishing rod handle.
	if (CreateGLTexture("Textures/Cork_Texture.jpg", "corkTexture"))
	{
		std::cout << "Loaded corkTexture (Cork_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load corkTexture (Cork_Texture.jpg)!\n";
	}

	// Load Tail_Texture.jpg into memory for the fish tail.
	if (CreateGLTexture("Textures/Tail_Texture.jpg", "tailTexture"))
	{
		std::cout << "Loaded tailTexture (Tail_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load tailTexture (Tail_Texture.jpg)!\n";
	}

	// Load Reel_Texture.jpg into memory for the fishing reel.
	if (CreateGLTexture("Textures/Reel_Texture.jpg", "reelTexture"))
	{
		std::cout << "Loaded reelTexture (Reel_Texture.jpg) successfully.\n";
	}
	else
	{
		std::cout << "Failed to load reelTexture (Reel_Texture.jpg)!\n";
	}

	BindGLTextures();
}

void SceneManager::DefineObjectMaterials() {

	// Wood material for the table.
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	woodMaterial.shininess = 5.0;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// Ceramic material for the mug.
	OBJECT_MATERIAL ceramicMaterial;
	ceramicMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	ceramicMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	ceramicMaterial.shininess = 30.0;
	ceramicMaterial.tag = "mug";
	m_objectMaterials.push_back(ceramicMaterial);

	// Tackle box material.
	OBJECT_MATERIAL tackleMaterial;
	tackleMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f);
	tackleMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	tackleMaterial.shininess = 15.0f;
	tackleMaterial.tag = "tackleBox";
	m_objectMaterials.push_back(tackleMaterial);

	// Fish material.
	OBJECT_MATERIAL fishMaterial;
	fishMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	fishMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	fishMaterial.shininess = 50.0f;
	fishMaterial.tag = "fish";
	m_objectMaterials.push_back(fishMaterial);

	// Cork material for fishing rod handle.
	OBJECT_MATERIAL corkMaterial;
	corkMaterial.diffuseColor = glm::vec3(0.8f, 0.6f, 0.4f);
	corkMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	corkMaterial.shininess = 5.0f;
	corkMaterial.tag = "cork";
	m_objectMaterials.push_back(corkMaterial);
}

void SceneManager::SetupSceneLights() {

	// Enable lighting in the shader.
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Main directional light (coming from above and slightly behind).
	glm::vec3 dir = glm::normalize(glm::vec3(0.3f, -1.0f, 0.5f));
	m_pShaderManager->setVec3Value("directionalLight.direction", dir);

	// Ambient low for contrast.
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.15f, 0.15f, 0.15f);

	// Strong main light.
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 1.0f, 0.95f, 0.8f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// Brighter point light to simulate sunlight.
	m_pShaderManager->setVec3Value("pointLights[0].position", -2.0f, 6.0f, -4.0f);

	// Increased ambient for overall brightness.
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.2f, 0.2f, 0.2f);

	// Stronger diffuse light.
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.0f, 0.98f, 0.9f);
	
	// Increased specular for sun-like highlights.
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.8f, 0.8f, 0.8f);

	// Adjusted attenuation for stronger reach.
	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.045f);
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.0075f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// Load the textures for the 3d scene.
	LoadSceneTextures();

	DefineObjectMaterials();

	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// Declare the variables for the transformations.
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(25.0f, 1.0f, 15.0f);
	positionXYZ = glm::vec3(0.0f, -0.5f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderTexture("tableTexture");
	SetTextureUVScale(4.0f, 3.0f);
	SetShaderMaterial("wood");

	// Draw the plane.
	m_basicMeshes->DrawPlaneMesh();

	/****************************************************************/

	// Coffee mug.
	scaleXYZ = glm::vec3(1.2f, 2.0f, 1.2f);
	positionXYZ = glm::vec3(4.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 30.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("mugTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("mug");
	
	// Draw the cylinder.
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	// Torus for mug handle.
	// Set the XYZ scale for the torus.
	scaleXYZ = glm::vec3(0.5f, 0.75f, 0.5f);

	// Set the XYZ position for the torus.
	positionXYZ = glm::vec3(5.25f, 1.0f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("mugTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("mug");

	// Draw the torus.
	m_basicMeshes->DrawTorusMesh();

	/****************************************************************/

	// Coffee liquid.
	scaleXYZ = glm::vec3(1.1f, 0.1f, 1.1f);
	positionXYZ = glm::vec3(4.0f, 1.91f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 30.0f, 0.0f, positionXYZ);
	SetShaderColor(0.2f, 0.1f, 0.05f, 1.0f);

	// Draw the cylinder.
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	// Tackle box.
	scaleXYZ = glm::vec3(4.0f, 2.0f, 2.5f);
	positionXYZ = glm::vec3(-4.0f, 1.0f, -1.0f);
	SetTransformations(scaleXYZ, 0.0f, 15.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("boxTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tackleBox");
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Fishing rod (cork handle).
	scaleXYZ = glm::vec3(0.3f, 3.0f, 0.3f);
	positionXYZ = glm::vec3(0.0f, 0.15f, 2.0f);
	SetTransformations(scaleXYZ, 0.0f, -20.0f, 90.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("corkTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cork");
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	// Rod shaft (thinner, darker section).
	scaleXYZ = glm::vec3(0.15f, 14.0f, 0.15f);
	positionXYZ = glm::vec3(1.25f, 0.15f, 2.0f);
	SetTransformations(scaleXYZ, 0.0f, -20.0f, 90.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("rodTexture");
	SetTextureUVScale(1.0, 3.0);
	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	// Fishing reel.
	scaleXYZ = glm::vec3(0.6f, 0.2f, 0.6f);

	// Rotate around X so the circular face is vertical.
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.65f, 0.05f, 2.75f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Reuse the tackle box material.
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("reelTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("tackleBox");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	// Side of fishing reel.
	scaleXYZ = glm::vec3(0.3f, 0.1f, 0.3f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.65f, 0.2f, 2.75f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// Reuse the tackle box material.
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	SetShaderMaterial("tackleBox");

	m_basicMeshes->DrawCylinderMesh();

	/****************************************************************/

	// Fish body (elongated sphere).
	scaleXYZ = glm::vec3(3.0f, 0.8f, 0.4f);
	positionXYZ = glm::vec3(0.0f, -0.4f, 6.0f);
	SetTransformations(scaleXYZ, 270.0f, 10.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("troutTexture");
	SetTextureUVScale(2.0, 1.0);
	SetShaderMaterial("fish");
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/

	// Fish eye.
	scaleXYZ = glm::vec3(0.15f, 0.15f, 0.05f);

	// Positioning relative to the fish body, adjusting for the fish's rotation.
	positionXYZ = glm::vec3(-2.3f, -0.2f, 6.1f);
	SetTransformations(scaleXYZ, 270.0f, 10.0f, 10.0f, positionXYZ);

	// Dark color for the eye.
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); 
	m_basicMeshes->DrawSphereMesh();

	/****************************************************************/

	// Fish tail using box mesh shaped into a triangle.
	// Thin and triangular shape.
	scaleXYZ = glm::vec3(0.8f, 0.1f, 0.8f); 
	positionXYZ = glm::vec3(2.95f, -0.4f, 5.475f);
	SetTransformations(scaleXYZ, 0.0f, 54.0f, 0.0f, positionXYZ);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	SetShaderTexture("tailTexture");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("fish");
	m_basicMeshes->DrawBoxMesh();

	/****************************************************************/

	// Fishing rod eyelets.
	// Adding multiple eyelets along the rod.
	// Positions along the rod.
	float eyeletPositions[] = { 2.5f, 4.5f, 6.5f, 8.5f, 10.5f }; 
	for (int i = 0; i < 5; i++) {
		scaleXYZ = glm::vec3(0.15f, 0.15f, 0.15f);

		// Calculate position along the angled rod.
		positionXYZ = glm::vec3(
			-15.0f + (eyeletPositions[i] * cos(glm::radians(-20.0f))), 
			0.15f,                                                     
			2.26f + (eyeletPositions[i] * sin(glm::radians(0.0f)))   
		);

		SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);

		// Dark metallic color for eyelets.
		SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f); 

		// Reusing tackle box material for metallic look.
		SetShaderMaterial("tackleBox"); 
		m_basicMeshes->DrawTorusMesh();
	}

	/****************************************************************/

	// Steam particles (small spheres with transparency).
	SetShaderColor(1.0f, 1.0f, 1.0f, 0.3f);
	float steamHeights[] = { 2.2f, 2.5f, 2.8f };
	float steamOffsets[] = { 0.1f, -0.1f, 0.0f };
	for (int i = 0; i < 3; i++) {
		scaleXYZ = glm::vec3(0.2f, 0.2f, 0.2f);
		positionXYZ = glm::vec3(4.0f + steamOffsets[i], steamHeights[i], 0.0f);
		SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
		m_basicMeshes->DrawSphereMesh();
	}
}