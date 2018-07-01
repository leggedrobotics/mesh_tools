#include "textured_mesh_visual.h"

#include <OGRE/OgreSubEntity.h>
#include <OGRE/OgreRenderOperation.h>
#include <OGRE/OgreTextureManager.h>
#include <OGRE/OgreHardwarePixelBuffer.h>
#include <OGRE/OgrePixelFormat.h>

// #include <rviz/display_context.h>
// #include <rviz/frame_manager.h>

#include <limits>
#include <stdint.h>



namespace rviz_mesh_plugin
{

  Ogre::ColourValue getRainbowColor(float value)
  {
      float r = 0.0f;
      float g = 0.0f;
      float b = 0.0f;

      value = std::min(value, 1.0f);
      value = std::max(value, 0.0f);

      float h = value * 5.0f + 1.0f;
      int i = floor(h);
      float f = h - i;
      if ( !(i&1) ) f = 1 - f; // if i is even
      float n = 1 - f;

      if      (i <= 1) r = n, g = 0, b = 1;
      else if (i == 2) r = 0, g = n, b = 1;
      else if (i == 3) r = 0, g = 1, b = n;
      else if (i == 4) r = n, g = 1, b = 0;
      else if (i >= 5) r = 1, g = n, b = 0;

      return Ogre::ColourValue(r, g, b, 1.0f);
  }

  TexturedMeshVisual::TexturedMeshVisual(
    rviz::DisplayContext* context,
      size_t displayID,
      size_t meshID,
      size_t randomID)
    : m_displayContext(context),
    m_prefix(displayID),
    m_postfix(meshID),
    m_random(randomID),
    m_vertex_normals_enabled(false),
    m_vertex_colors_enabled(false),
    m_materials_enabled(false),
    m_texture_coords_enabled(false),
    m_normalsScalingFactor(1)
  {

    ROS_INFO("Creating TexturedMeshVisual %lu_TexturedMesh_%lu_%lu",m_prefix, m_postfix, m_random);

    // get or create the scene node
    Ogre::SceneManager* sceneManager = m_displayContext->getSceneManager();
    Ogre::SceneNode* rootNode = sceneManager->getRootSceneNode();

    std::stringstream strstream;
    strstream << "TexturedMeshScene" << m_random;
    std::string sceneId = strstream.str();
    if (sceneManager->hasSceneNode(sceneId))
    {
      //ROS_INFO("Attaching to scene: %s", sceneId);
      m_sceneNode = (Ogre::SceneNode*)(rootNode->getChild(sceneId));
    }
    else
    {
      //ROS_INFO("Creating new scene: %s", sceneId);
      m_sceneNode = rootNode->createChildSceneNode(sceneId);
    }

    // create manual objects and attach them to the scene node
    std::stringstream sstm;
    sstm << m_prefix << "_TriangleMesh_" << m_postfix << "_" << m_random;
    m_mesh = sceneManager->createManualObject(sstm.str());
    m_mesh->setDynamic(false);
    m_sceneNode->attachObject(m_mesh);

    std::stringstream sstmNormals;
    sstmNormals << m_prefix << "_Normals_" << m_postfix << "_" << m_random;
    m_normals = sceneManager->createManualObject(sstmNormals.str());
    m_normals->setDynamic(false);
    m_sceneNode->attachObject(m_normals);

    std::stringstream sstmTexturedMesh;
    sstmTexturedMesh << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random;
    m_texturedMesh = sceneManager->createManualObject(sstmTexturedMesh.str());
    m_texturedMesh->setDynamic(false);
    m_sceneNode->attachObject(m_texturedMesh);

    std::stringstream sstmNoTexCluMesh;
    sstmNoTexCluMesh << m_prefix << "_NoTexCluMesh_" << m_postfix << "_" << m_random;
    m_noTexCluMesh = sceneManager->createManualObject(sstmNoTexCluMesh.str());
    m_noTexCluMesh->setDynamic(false);
    m_sceneNode->attachObject(m_noTexCluMesh);

    std::stringstream sstmVertexCostsMesh;
    sstmVertexCostsMesh << m_prefix << "_VertexCostsMesh_" << m_postfix << "_" << m_random;
    m_vertexCostsMesh = sceneManager->createManualObject(sstmVertexCostsMesh.str());
    m_vertexCostsMesh->setDynamic(false);
    m_sceneNode->attachObject(m_vertexCostsMesh);
  }

  TexturedMeshVisual::~TexturedMeshVisual()
  {
    ROS_INFO("Destroying TexturedMeshVisual %lu_TexturedMesh_%lu_%lu",m_prefix, m_postfix, m_random);

    reset();

    std::stringstream sstm;
    sstm << m_prefix << "_TriangleMesh_" << m_postfix << "_" << m_random;
    m_displayContext->getSceneManager()->destroyManualObject(sstm.str());

    std::stringstream sstmNormals;
    sstmNormals << m_prefix << "_Normals_" << m_postfix << "_" << m_random;
    m_displayContext->getSceneManager()->destroyManualObject(sstmNormals.str());

    std::stringstream sstmTexturedMesh;
    sstmTexturedMesh << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random;
    m_displayContext->getSceneManager()->destroyManualObject(sstmTexturedMesh.str());

    std::stringstream sstmNoTexCluMesh;
    sstmNoTexCluMesh << m_prefix << "_NoTexCluMesh_" << m_postfix << "_" << m_random;
    m_displayContext->getSceneManager()->destroyManualObject(sstmNoTexCluMesh.str());

    std::stringstream sstmVertexCostsMesh;
    sstmVertexCostsMesh << m_prefix << "_VertexCostsMesh_" << m_postfix << "_" << m_random;
    m_displayContext->getSceneManager()->destroyManualObject(sstmVertexCostsMesh.str());

    m_displayContext->getSceneManager()->destroySceneNode(m_sceneNode);
    sstm.str("");
    sstm.flush();
  }

  void TexturedMeshVisual::reset()
  {

    ROS_INFO("Resetting TexturedMeshVisual %lu_TexturedMesh_%lu_%lu",m_prefix, m_postfix, m_random);


    std::stringstream sstm;

    sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "GeneralMaterial_";
    Ogre::MaterialManager::getSingleton().unload(sstm.str());
    Ogre::MaterialManager::getSingleton().remove(sstm.str());
    sstm.str("");
    sstm.clear();

    if (m_vertex_colors_enabled)
    {
      sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "Material_" << 1;
      Ogre::MaterialManager::getSingleton().unload(sstm.str());
      Ogre::MaterialManager::getSingleton().remove(sstm.str());
      sstm.str("");
      sstm.clear();
    }

    sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "NormalMaterial";
    Ogre::MaterialManager::getSingleton().unload(sstm.str());
    Ogre::MaterialManager::getSingleton().remove(sstm.str());
    sstm.str("");
    sstm.clear();

    for (Ogre::MaterialPtr textureMaterial : m_textureMaterials)
    {
      Ogre::MaterialManager::getSingleton().unload(textureMaterial->getName());
      Ogre::MaterialManager::getSingleton().remove(textureMaterial->getName());
    }

    if (!m_noTexCluMaterial.isNull())
    {
      Ogre::MaterialManager::getSingleton().unload(m_noTexCluMaterial->getName());
      Ogre::MaterialManager::getSingleton().remove(m_noTexCluMaterial->getName());
    }

    if (!m_vertexCostMaterial.isNull())
    {
      Ogre::MaterialManager::getSingleton().unload(m_vertexCostMaterial->getName());
      Ogre::MaterialManager::getSingleton().remove(m_vertexCostMaterial->getName());
    }

    m_mesh->clear();
    m_normals->clear();
    m_texturedMesh->clear();
    m_noTexCluMesh->clear();
    m_vertexCostsMesh->clear();
    sstm.str("");
    sstm.flush();

    m_meshGeneralMaterial.setNull();
    m_normalMaterial.setNull();
    m_noTexCluMaterial.setNull();
    m_textureMaterials.clear();
    m_vertexCostMaterial.setNull();

    m_images.clear();

    m_meshUuid = "";
    m_vertexColorsUuid = "";
    m_vertexCostsUuid = "";
    m_materialsUuid = "";

    m_vertex_colors_enabled = false;
    m_materials_enabled = false;
    m_texture_coords_enabled = false;
    m_textures_enabled = false;
    m_vertex_costs_enabled = false;
  }

  void TexturedMeshVisual::showWireframe(
    Ogre::Pass* pass,
    Ogre::ColourValue wireframeColor,
    float wireframeAlpha
  )
  {
    pass->setAmbient(
      Ogre::ColourValue(
        wireframeColor.r,
        wireframeColor.g,
        wireframeColor.b,
        wireframeAlpha
      )
    );
    pass->setDiffuse(
      Ogre::ColourValue(
        wireframeColor.r,
        wireframeColor.g,
        wireframeColor.b,
        wireframeAlpha
      )
    );

    if (wireframeAlpha < 1.0)
    {
      pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
      pass->setDepthWriteEnabled(false);
    }
    pass->setPolygonMode(Ogre::PM_WIREFRAME);
    pass->setCullingMode(Ogre::CULL_NONE);
  }

  void TexturedMeshVisual::showFaces(
    Ogre::Pass* pass,
    Ogre::ColourValue facesColor,
    float facesAlpha,
    bool useVertexColors
  )
  {

    pass->setDiffuse(
      Ogre::ColourValue(
        facesColor.r,
        facesColor.g,
        facesColor.b,
        facesAlpha
      )
    );
    pass->setSelfIllumination(facesColor.r, facesColor.g, facesColor.b);


    if (useVertexColors)
    {
      pass->setLightingEnabled(false);
    }

    if (facesAlpha < 1.0)
    {
      pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
      pass->setDepthWriteEnabled(false);
    }
    pass->setPolygonMode(Ogre::PM_SOLID);
    pass->setCullingMode(Ogre::CULL_NONE);
  }

  void TexturedMeshVisual::showNormals(
    Ogre::Pass* pass,
    Ogre::ColourValue normalsColor,
    float normalsAlpha
  )
  {

    pass->setSelfIllumination(normalsColor.r, normalsColor.g, normalsColor.b);
    pass->setDiffuse(
      Ogre::ColourValue(
        normalsColor.r,
        normalsColor.g,
        normalsColor.b,
        normalsAlpha
      )
    );
    if (normalsAlpha < 1.0)
    {
      pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
      pass->setDepthWriteEnabled(false);
    }
    pass->setPolygonMode(Ogre::PM_SOLID);
    pass->setCullingMode(Ogre::CULL_NONE);
  }

  void TexturedMeshVisual::updateMaterial(
    bool showWireframe,
    Ogre::ColourValue wireframeColor,
    float wireframeAlpha,
    bool showFaces,
    Ogre::ColourValue facesColor,
    float facesAlpha,
    bool useVertexColors,
    bool showVertexCosts,
    bool showTextures,
    bool showTexturedFacesOnly,
    bool showNormals,
    Ogre::ColourValue normalsColor,
    float normalsAlpha,
    float normalsScalingFactor
  )
  {
    // remove all passes
    if (!m_meshGeneralMaterial.isNull())
    {
      m_meshGeneralMaterial->getTechnique(0)->removeAllPasses();
    }

    if (!m_normalMaterial.isNull())
    {
      m_normalMaterial->getTechnique(0)->removeAllPasses();
    }

    m_texturedMesh->setVisible(false);
    m_noTexCluMesh->setVisible(false);
    m_vertexCostsMesh->setVisible(false);

    // if the material exists and the textures are not enabled
    // we can use the general mesh with the m_meshGeneralMaterial
    if (!m_meshGeneralMaterial.isNull() && !showTextures && !showVertexCosts)
    {
      Ogre::Technique* tech = m_meshGeneralMaterial->getTechnique(0);

      if (showFaces)
      {
        this->showFaces(tech->createPass(), facesColor, facesAlpha, useVertexColors);
      }
    }

    // if there are vertex costs and the vertex cost are enabled
    // the mesh with the colors calculated from vertex costs is made visible
    if (m_vertex_costs_enabled && showVertexCosts)
    {
      m_vertexCostsMesh->setVisible(true);
    }

    // if there are materials or textures the mesh with texture coordinates that
    // uses the material and texture materials is made visible
    if ((m_materials_enabled || m_textures_enabled) && showTextures)
    {
      m_texturedMesh->setVisible(true);
      m_noTexCluMesh->setVisible(!showTexturedFacesOnly); // TODO: dynamisch
    }

    if (showWireframe)
    {
      Ogre::Technique* tech = m_meshGeneralMaterial->getTechnique(0);
      this->showWireframe(tech->createPass(), wireframeColor, wireframeAlpha);
    }

    if (!m_normalMaterial.isNull())
    {
      if (showNormals)
      {
        Ogre::Technique* tech = m_normalMaterial->getTechnique(0);
        this->showNormals(tech->createPass(), normalsColor, normalsAlpha);
        updateNormals(normalsScalingFactor);
      }
    }
  }

void TexturedMeshVisual::updateNormals(float ScalingFactor)
{
  Ogre::VertexData* vertexData;
  const Ogre::VertexElement* vertexElement;
  Ogre::HardwareVertexBufferSharedPtr vertexBuffer;
  unsigned char* vertexChar;
  float* vertexFloat;

  vertexData = m_normals->getSection(0)->getRenderOperation()->vertexData;
  vertexElement = vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
  vertexBuffer = vertexData->vertexBufferBinding->getBuffer(vertexElement->getSource());
  vertexChar = static_cast<unsigned char*>(vertexBuffer->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

  size_t halfVertexCount = vertexData->vertexCount / 2;
  Ogre::Vector3* vertices = new Ogre::Vector3[halfVertexCount];
  Ogre::Vector3* normals = new Ogre::Vector3[halfVertexCount];

  for (
    size_t i = 0, vIndex = 0, nIndex = 0;
    i < vertexData->vertexCount;
    i++, vertexChar += vertexBuffer->getVertexSize()
  )
  {
    vertexElement->baseVertexPointerToElement(vertexChar, &vertexFloat);
    Ogre::Vector3 tempVector(vertexFloat[0], vertexFloat[1], vertexFloat[2]);

    if (i % 2 == 0)
    {
      vertices[vIndex] = tempVector;
      vIndex++;
    }
    else
    {
      normals[nIndex] = (tempVector - vertices[nIndex]) / m_normalsScalingFactor;
      nIndex++;
    }
  }
  vertexBuffer->unlock();

  m_normals->beginUpdate(0);
  for (size_t i = 0; i < halfVertexCount; i++)
  {
    m_normals->position(vertices[i].x, vertices[i].y, vertices[i].z);
    m_normals->position(
      vertices[i].x + ScalingFactor * normals[i].x,
      vertices[i].y + ScalingFactor * normals[i].y,
      vertices[i].z + ScalingFactor * normals[i].z
    );
  }
  m_normals->end();
  delete [] vertices;
  delete [] normals;
  m_normalsScalingFactor = ScalingFactor;
}

void TexturedMeshVisual::enteringGeneralTriangleMesh(const mesh_msgs::MeshGeometry& mesh)
{

  std::stringstream sstm;

  sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "GeneralMaterial_";

  m_meshGeneralMaterial =
    Ogre::MaterialManager::getSingleton().create(
      sstm.str(),
      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
      true
    );

  // start entering data
  m_mesh->begin(sstm.str(), Ogre::RenderOperation::OT_TRIANGLE_LIST);

  // write vertices
  // write vertex normals(if enabled)
  for (size_t i = 0; i < mesh.vertices.size(); i++)
  {
    // write vertices
    m_mesh->position(
      mesh.vertices[i].x,
      mesh.vertices[i].y,
      mesh.vertices[i].z
    );

    // write vertex normals, if enabled
    if (m_vertex_normals_enabled)
    {
      m_mesh->normal(
        mesh.vertex_normals[i].x,
        mesh.vertex_normals[i].y,
        mesh.vertex_normals[i].z
      );
    }
  }

  // write triangles
  for (size_t i = 0; i < mesh.faces.size(); i++)
  {
    m_mesh->triangle(
      mesh.faces[i].vertex_indices[0],
      mesh.faces[i].vertex_indices[1],
      mesh.faces[i].vertex_indices[2]
    );
  }

  // finish entering data
  m_mesh->end();

}

void TexturedMeshVisual::enteringColoredTriangleMesh(
  const mesh_msgs::MeshGeometry& mesh,
  const mesh_msgs::MeshVertexColors& vertexColors)
{

  if (m_meshGeneralMaterial.isNull())
  {
    std::stringstream sstm;
    sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "GeneralMaterial_";

    m_meshGeneralMaterial =
      Ogre::MaterialManager::getSingleton().create(
        sstm.str(),
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        true
      );
  }

  // start entering data
  m_mesh->begin(m_meshGeneralMaterial->getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST);

  // write vertices
  // write vertex colors
  // write vertex normals(if enabled)
  for (size_t i = 0; i < mesh.vertices.size(); i++)
  {
    // write vertices
    m_mesh->position(mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);

    // write vertex colors
    m_mesh->colour(
      vertexColors.vertex_colors[i].r,
      vertexColors.vertex_colors[i].g,
      vertexColors.vertex_colors[i].b,
      vertexColors.vertex_colors[i].a
    );

    // write vertex normals, if enabled
    if (m_vertex_normals_enabled)
    {
      m_mesh->normal(
        mesh.vertex_normals[i].x,
        mesh.vertex_normals[i].y,
        mesh.vertex_normals[i].z
      );
    }
  }

  // write triangles
  for (size_t i = 0; i < mesh.faces.size(); i++)
  {
    m_mesh->triangle(
      mesh.faces[i].vertex_indices[0],
      mesh.faces[i].vertex_indices[1],
      mesh.faces[i].vertex_indices[2]
    );
  }

  // finish entering data
  m_mesh->end();

}

void TexturedMeshVisual::enteringTriangleMeshWithVertexCosts(
  const mesh_msgs::MeshGeometry& mesh,
  const mesh_msgs::MeshVertexCosts& vertexCosts,
  int costColorType
)
{

  // Calculate maximum value for vertex costs
  float maxCost = 0.0f;
  float minCost = std::numeric_limits<float>::max();
  for (float cost : vertexCosts.costs)
  {
    maxCost = cost > maxCost ? cost : maxCost;
    minCost = cost < minCost ? cost : minCost;
  }

  enteringTriangleMeshWithVertexCosts(mesh, vertexCosts, costColorType, minCost, maxCost);
}

void TexturedMeshVisual::enteringTriangleMeshWithVertexCosts(
  const mesh_msgs::MeshGeometry& mesh,
  const mesh_msgs::MeshVertexCosts& vertexCosts,
  int costColorType,
  float minCost,
  float maxCost
)
{

  float range = maxCost - minCost;
  if (range <= 0)
  {
    ROS_ERROR("Illegal vertex cost limits!");
    return;
  }

  if (m_vertexCostMaterial.isNull())
  {
    std::stringstream sstm;
    sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "VertexCostMaterial_";

    m_vertexCostMaterial =
      Ogre::MaterialManager::getSingleton().create(
        sstm.str(),
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        true
      );

    Ogre::Pass* pass = m_vertexCostMaterial->getTechnique(0)->getPass(0);
    pass->setCullingMode(Ogre::CULL_NONE);
    pass->setLightingEnabled(false);
  }

  // start entering data
  m_vertexCostsMesh->begin(m_vertexCostMaterial->getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST);

  // write vertices
  // write vertex colors
  // write vertex normals(if enabled)
  for (size_t i = 0; i < mesh.vertices.size(); i++)
  {
    // write vertices
    m_vertexCostsMesh->position(mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);

    // write vertex colors that are calculated from the cost values
    float normalizedCost = (minCost + vertexCosts.costs[i]) / range;
    m_vertexCostsMesh->colour(calculateColorFromCost(normalizedCost, costColorType));

    // write vertex normals, if enabled
    if (m_vertex_normals_enabled)
    {
      m_vertexCostsMesh->normal(
        mesh.vertex_normals[i].x,
        mesh.vertex_normals[i].y,
        mesh.vertex_normals[i].z
      );
    }
  }

  // write triangles
  for (size_t i = 0; i < mesh.faces.size(); i++)
  {
    m_vertexCostsMesh->triangle(
      mesh.faces[i].vertex_indices[0],
      mesh.faces[i].vertex_indices[1],
      mesh.faces[i].vertex_indices[2]
    );
  }

  // finish entering data
  m_vertexCostsMesh->end();

}

void TexturedMeshVisual::enteringTexturedTriangleMesh(
    const mesh_msgs::MeshGeometry& mesh,
    const mesh_msgs::MeshMaterials& meshMaterials
)
{
  size_t clusterCounter = meshMaterials.clusters.size();
  size_t materialCounter = meshMaterials.materials.size();
  size_t textureIndex = 0;

  std::stringstream sstm;
  sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "NoTexCluMaterial_";
  m_noTexCluMaterial = Ogre::MaterialManager::getSingleton().create(
    sstm.str(),
    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
    true
  );

  Ogre::Pass* pass = m_noTexCluMaterial->getTechnique(0)->getPass(0);
  pass->setCullingMode(Ogre::CULL_NONE);
  pass->setLightingEnabled(false);


  m_noTexCluMesh->begin(m_noTexCluMaterial->getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST);

  size_t noTexCluVertexCount = 0;

  for (size_t clusterIndex = 0; clusterIndex < clusterCounter; clusterIndex++)
  {

    mesh_msgs::Cluster cluster = meshMaterials.clusters[clusterIndex];

    uint32_t materialIndex = meshMaterials.cluster_materials[clusterIndex];
    mesh_msgs::Material material = meshMaterials.materials[materialIndex];
    bool hasTexture = material.has_texture;


    // if the material has a texture, create an ogre texture and load the image
    if (hasTexture)
    {
      std::stringstream sstm;
      sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "TextureMaterial_" << textureIndex;
      m_textureMaterials.push_back(
        Ogre::MaterialManager::getSingleton().create(
          sstm.str(),
          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          true
        )
      );

      // set some rendering options for textured clusters
      Ogre::Pass* pass = m_textureMaterials[textureIndex]->getTechnique(0)->getPass(0);
      // pass->setTextureFiltering(Ogre::TFO_NONE);
      pass->setCullingMode(Ogre::CULL_NONE);
      pass->setLightingEnabled(false);

      // check if image was already loaded
      // this is the case if the vector of images doesn't contain this element yet or
      // if the image was only default constructed, in which case its width will be 0
      if (m_images.size() < textureIndex + 1 || m_images[textureIndex].getWidth() == 0)
      {
        ROS_INFO("Texture with index %lu not loaded yet", textureIndex);
      }
      else
      {
        loadImageIntoTextureMaterial(textureIndex);
      }
    }


    if (hasTexture)
    {
      // start entering data
      m_texturedMesh->begin(m_textureMaterials[textureIndex]->getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST);
      textureIndex++;

      // write vertices for each triangle
      // write texture coordinates
      // write vertex normals (if enabled)

      size_t triangleVertexCount = 0;
      for (size_t i = 0; i < cluster.face_indices.size(); i++)
      {
        uint32_t faceIndex = cluster.face_indices[i];
        // write three triangle vertices
        for (size_t j = 0; j < 3; j++)
        {
          int vertexIndex = mesh.faces[faceIndex].vertex_indices[j];
          // write vertex positions
          m_texturedMesh->position(
            mesh.vertices[vertexIndex].x,
            mesh.vertices[vertexIndex].y,
            mesh.vertices[vertexIndex].z
          );
          // write texture coordinates
          m_texturedMesh->textureCoord(
            meshMaterials.vertex_tex_coords[vertexIndex].u,
            1 - meshMaterials.vertex_tex_coords[vertexIndex].v
          );

          // write vertex normals, if enabled
          if (m_vertex_normals_enabled)
          {
            m_texturedMesh->normal(
              mesh.vertex_normals[vertexIndex].x,
              mesh.vertex_normals[vertexIndex].y,
              mesh.vertex_normals[vertexIndex].z
            );
          }
        }
        // write the three triangle vertex indices
        m_texturedMesh->triangle(
          triangleVertexCount,
          triangleVertexCount + 1,
          triangleVertexCount + 2
        );
        triangleVertexCount += 3;
      }

      // finish entering data
      m_texturedMesh->end();

    }
    else
    {
      // write vertices for each triangle to enable a coloring for each triangle
      // write triangle colors as vertex colours
      // write vertex normals (if enabled)

      size_t triangleVertexCount = 0;
      for (size_t i = 0; i < cluster.face_indices.size(); i++)
      {
        uint32_t faceIndex = cluster.face_indices[i];
        // write three triangle vertices
        for (size_t j = 0; j < 3; j++)
        {
          int vertexIndex = mesh.faces[faceIndex].vertex_indices[j];
          // write vertex positions
          m_noTexCluMesh->position(
            mesh.vertices[vertexIndex].x,
            mesh.vertices[vertexIndex].y,
            mesh.vertices[vertexIndex].z
          );

          // write triangle colors
          m_noTexCluMesh->colour(
            material.color.r,
            material.color.g,
            material.color.b,
            material.color.a
          );

          // write vertex normals, if enabled
          if (m_vertex_normals_enabled)
          {
            m_noTexCluMesh->normal(
              mesh.vertex_normals[vertexIndex].x,
              mesh.vertex_normals[vertexIndex].y,
              mesh.vertex_normals[vertexIndex].z
            );
          }
        }
        // write the three triangle vertex indices
        m_noTexCluMesh->triangle(
          noTexCluVertexCount,
          noTexCluVertexCount + 1,
          noTexCluVertexCount + 2
        );
        noTexCluVertexCount += 3;
      }

    }
  }

  m_noTexCluMesh->end();
}

void TexturedMeshVisual::enteringNormals(const mesh_msgs::MeshGeometry& mesh)
{

  if (!m_vertex_normals_enabled)
  {
    return;
  }

  std::stringstream sstm;
  sstm << m_prefix << "_TexturedMesh_" << m_postfix << "_" << m_random << "NormalMaterial";
  m_normalMaterial = Ogre::MaterialManager::getSingleton().create(
    sstm.str(),
    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
    true
  );

  // Create pointNormals
  m_normals->begin(sstm.str(), Ogre::RenderOperation::OT_LINE_LIST);

  // Vertices
  for (size_t i = 0; i < mesh.vertex_normals.size(); i++)
  {
    m_normals->position(mesh.vertices[i].x, mesh.vertices[i].y, mesh.vertices[i].z);
    m_normals->position(
      mesh.vertices[i].x + m_normalsScalingFactor * mesh.vertex_normals[i].x,
      mesh.vertices[i].y + m_normalsScalingFactor * mesh.vertex_normals[i].y,
      mesh.vertices[i].z + m_normalsScalingFactor * mesh.vertex_normals[i].z
    );
    // add line to index buffer
    m_normals->index(2 * i);
    m_normals->index(2 * i + 1);
  }
  m_normals->end();

}

bool TexturedMeshVisual::setGeometry(const mesh_msgs::MeshGeometryStamped::ConstPtr& meshMsg)
{
  reset();

  // for a better legibility of the code
  const mesh_msgs::MeshGeometry& mesh = meshMsg->mesh_geometry;
  m_meshMsg = meshMsg->mesh_geometry;
  m_meshUuid = meshMsg->uuid;

  // default: vertex colors are optional and therefore disabled
  m_vertex_colors_enabled = false;

  // default: textures and texture_coords are optional and therefore disabled
  m_textures_enabled = false;
  m_texture_coords_enabled = false;

  // default: vertex normals are optional and therefore disabled
  m_vertex_normals_enabled = false;

  // default: vertex costs are optional and therefore disabled
  m_vertex_costs_enabled = false;

  // check if there are enough vertices given
  if (mesh.vertices.size() < 3)
  {
    ROS_WARN("Received not enough vertices, can't create mesh!");
    return false;
  }

  // defines the buffer sizes
  int vertex_count = mesh.vertices.size();
  int index_count = mesh.faces.size() * 3;


  // vertex normals
  // check if there are vertex normals for each vertex
  if (mesh.vertex_normals.size() == mesh.vertices.size())
  {
    ROS_INFO("Received %lu vertex normals.", mesh.vertex_normals.size());
    m_vertex_normals_enabled = true;
  }
  else if (mesh.vertex_normals.size() > 0)
  {
    ROS_WARN("Received not as much vertex normals as vertices, ignoring vertex normals!");
  }

  // avoid memory reallocation
  m_mesh->estimateVertexCount(vertex_count);
  m_mesh->estimateIndexCount(index_count);
  m_normals->estimateVertexCount(mesh.vertices.size() * 2);
  m_normals->estimateIndexCount(mesh.vertices.size() * 2);
  // m_texturedMesh->estimateVertexCount(vertex_count * 3);
  // m_texturedMesh->estimateIndexCount(index_count);

  // entering a general triangle mesh into the internal buffer
  enteringGeneralTriangleMesh(mesh);

  // entering the normals into the internal buffer
  if (m_vertex_normals_enabled)
  {
    enteringNormals(mesh);
  }

  return true;
}

bool TexturedMeshVisual::setVertexColors(const mesh_msgs::MeshVertexColorsStamped::ConstPtr& vertexColorsMsg)
{
  //check if these MeshVertexColors belong to the current mesh and were not already loaded
  if(m_meshUuid != vertexColorsMsg->uuid)
  {
    ROS_WARN("Can't add vertex colors, uuids do not match.");
    return false;
  }
  // check if the vertex colors for this mesh were already set
  if(m_vertexColorsUuid == vertexColorsMsg->uuid)
  {
    ROS_WARN("Can't add vertex colors, already received vertex colors for this mesh.");
    return false;
  }

  const mesh_msgs::MeshVertexColors vertexColors = vertexColorsMsg->mesh_vertex_colors;

  // check if there are vertex colors for each vertex
  if (vertexColors.vertex_colors.size() == m_meshMsg.vertices.size())
  {
    ROS_INFO("Received %lu vertex colors.", vertexColors.vertex_colors.size());
    m_vertex_colors_enabled = true;
  }
  else
  {
    ROS_WARN("Received not as much vertex colors as vertices, ignoring the vertex colors!");
    return false;
  }

  enteringColoredTriangleMesh(m_meshMsg, vertexColors);

  m_vertexColorsUuid = vertexColorsMsg->uuid;

  return true;
}

bool TexturedMeshVisual::setVertexCosts(
  const mesh_msgs::MeshVertexCostsStamped::ConstPtr& vertexCostsMsg,
  int costColorType
)
{
  //check if these MeshVertexCosts belong to the current mesh and were not already loaded
  if(m_meshUuid != vertexCostsMsg->uuid)
  {
    ROS_WARN("Can't add vertex costs, uuids do not match.");
    return false;
  }
  // check if the vertex costs for this mesh were already set
  if(m_vertexCostsUuid == vertexCostsMsg->uuid)
  {
    ROS_WARN("Already received vertex costs for this mesh. Overwriting previously received vertex costs!");
  }

  const mesh_msgs::MeshVertexCosts vertexCosts = vertexCostsMsg->mesh_vertex_costs;

  // check if there are vertex costs for each vertex
  if (vertexCosts.costs.size() == m_meshMsg.vertices.size())
  {
    ROS_INFO("Received %lu vertex costs.", vertexCosts.costs.size());
    m_vertex_costs_enabled = true;
  }
  else
  {
    ROS_WARN("Received not as much vertex costs as vertices, ignoring the vertex costs!");
    return false;
  }

  enteringTriangleMeshWithVertexCosts(m_meshMsg, vertexCosts, costColorType);

  m_vertexCostsUuid = vertexCostsMsg->uuid;

  return true;
}

bool TexturedMeshVisual::setVertexCosts(
  const mesh_msgs::MeshVertexCostsStamped::ConstPtr& vertexCostsMsg,
  int costColorType,
  float minCost,
  float maxCost
)
{
  //check if these MeshVertexCosts belong to the current mesh and were not already loaded
  if(m_meshUuid != vertexCostsMsg->uuid)
  {
    ROS_WARN("Can't add vertex costs, uuids do not match.");
    return false;
  }
  // check if the vertex costs for this mesh were already set
  if(m_vertexCostsUuid == vertexCostsMsg->uuid)
  {
    ROS_WARN("Already received vertex costs for this mesh. Overwriting previously received vertex costs!");
  }

  const mesh_msgs::MeshVertexCosts vertexCosts = vertexCostsMsg->mesh_vertex_costs;

  // check if there are vertex costs for each vertex
  if (vertexCosts.costs.size() == m_meshMsg.vertices.size())
  {
    ROS_INFO("Received %lu vertex costs.", vertexCosts.costs.size());
    m_vertex_costs_enabled = true;
  }
  else
  {
    ROS_WARN("Received not as much vertex costs as vertices, ignoring the vertex costs!");
    return false;
  }

  enteringTriangleMeshWithVertexCosts(m_meshMsg, vertexCosts, costColorType, minCost, maxCost);

  m_vertexCostsUuid = vertexCostsMsg->uuid;

  return true;
}

bool TexturedMeshVisual::setMaterials(const mesh_msgs::MeshMaterialsStamped::ConstPtr& materialMsg)
{
  //check if these MeshMaterials belong to the current mesh and were not already loaded
  if(m_meshUuid != materialMsg->uuid)
  {
    ROS_WARN("Can't add materials, uuids do not match.");
    return false;
  }
    //check if the MeshMaterials were already set for this mesh
  if(m_materialsUuid == materialMsg->uuid)
  {
    ROS_WARN("Can't add materials, already received materials for this mesh.");
    return false;
  }

  mesh_msgs::MeshMaterials meshMaterials = materialMsg->mesh_materials;

  // check if there is a material index for each cluster
  if (meshMaterials.clusters.size() == meshMaterials.cluster_materials.size())
  {
    ROS_INFO("Received %lu clusters.", meshMaterials.clusters.size());
    m_materials_enabled = true; // enable textures
  }
  else
  {
    ROS_WARN("Received unmatching numbers of clusters and material indices, ignoring materials!");
    return false;
  }

  // texture coords
  // check if there are texture coords for each vertex
  if (meshMaterials.vertex_tex_coords.size() == m_meshMsg.vertices.size())
  {
    ROS_INFO("Received %lu texture coords.", meshMaterials.vertex_tex_coords.size());
    m_texture_coords_enabled = true; // enable texture coords
    m_textures_enabled = true;
  }
  else if (meshMaterials.vertex_tex_coords.size() > 0)
  {
    ROS_WARN("Received not as much texture coords as vertices, ignoring texture coords!");
  }

  enteringTexturedTriangleMesh(m_meshMsg, meshMaterials);

  m_materialsUuid = materialMsg->uuid;

  return true;
}

bool TexturedMeshVisual::addTexture(const mesh_msgs::Texture::ConstPtr& textureMsg)
{
  if(m_meshUuid != textureMsg->uuid || m_materialsUuid != textureMsg->uuid)
  {
    ROS_WARN("Can't add texture, uuids do not match.");
    return false;
  }

  size_t textureIndex = textureMsg->texture_index;

  uint32_t width = textureMsg->image.width;
  uint32_t height = textureMsg->image.height;
  uint32_t step = textureMsg->image.step;
  std::vector<uint8_t> data = textureMsg->image.data;

  uint32_t dataSize = height * step;

  uchar* imageData = new uchar[dataSize];
  std::memcpy(imageData, &data[0], dataSize);

  Ogre::PixelFormat pixelFormat = getOgrePixelFormatFromRosString(textureMsg->image.encoding);

  Ogre::Image image = Ogre::Image();
  // image.loadDynamicImage(&data[0], width, height, 1, pixelFormat, false);
  image.loadDynamicImage(imageData, width, height, 1, pixelFormat, false);
  m_images.insert(m_images.begin() + textureIndex, image);

  if (m_textureMaterials.size() >= textureIndex + 1)
  {
    loadImageIntoTextureMaterial(textureIndex);
    return true;
  }
  else
  {
    ROS_WARN("Can't load image into texture material, material does not exist!");
    return false;
  }

}

Ogre::PixelFormat TexturedMeshVisual::getOgrePixelFormatFromRosString(std::string encoding)
{
  if (encoding == "rgba8")
  {
    return Ogre::PF_BYTE_RGBA;
  }
  else if (encoding == "rgb8")
  {
    return Ogre::PF_BYTE_RGB;
  }

  ROS_WARN("Unknown texture encoding! Using Ogre::PF_UNKNOWN");
  return Ogre::PF_UNKNOWN;
}

void TexturedMeshVisual::loadImageIntoTextureMaterial(size_t textureIndex)
{
  std::stringstream textureNameStream;
  textureNameStream << m_prefix << "_Texture" << textureIndex << "_" << m_postfix << "_" << m_random;

  Ogre::TexturePtr texturePtr = Ogre::TextureManager::getSingleton().createManual(
    textureNameStream.str(),
    Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
    Ogre::TEX_TYPE_2D,
    m_images[textureIndex].getWidth(),
    m_images[textureIndex].getHeight(),
    0,
    m_images[textureIndex].getFormat()
  );

  texturePtr->loadImage(m_images[textureIndex]);

  Ogre::Pass* pass = m_textureMaterials[textureIndex]->getTechnique(0)->getPass(0);
  pass->removeAllTextureUnitStates();
  pass->createTextureUnitState()->addFrameTextureName(textureNameStream.str());
}

Ogre::ColourValue TexturedMeshVisual::calculateColorFromCost(float cost, int costColorType)
{
  Ogre::ColourValue color;

  switch (costColorType)
  {
    case 0: // rainbow
      return getRainbowColor(cost);
    case 1: // red green
      // calculate a color that is green for 0, yellow for 0.5 and red for 1
      color.r = cost * 2;
      color.r = color.r > 1.0f ? 1.0f : color.r;
      color.g = (1.0f - cost) * 2;
      color.g = color.g > 1.0f ? 1.0f : color.g;
      color.b = 0.0f;
      color.a = 1.0;
      return color;
    default:
      break;
  }
  // default
  return getRainbowColor(cost);

}

void TexturedMeshVisual::setFramePosition(const Ogre::Vector3& position)
{
  m_sceneNode->setPosition(position);
}

void TexturedMeshVisual::setFrameOrientation(const Ogre::Quaternion& orientation)
{
  m_sceneNode->setOrientation(orientation);
}

} // end namespace rviz_mesh_plugin
