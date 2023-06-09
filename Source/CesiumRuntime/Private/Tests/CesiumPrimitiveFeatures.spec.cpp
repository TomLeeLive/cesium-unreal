#include "CesiumPrimitiveFeatures.h"
#include "CesiumFeatureIdSpecUtility.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "Misc/AutomationTest.h"

using namespace CesiumGltf;

BEGIN_DEFINE_SPEC(
    FCesiumPrimitiveFeaturesSpec,
    "Cesium.PrimitiveFeatures",
    EAutomationTestFlags::ApplicationContextMask |
        EAutomationTestFlags::ProductFilter)
Model model;
MeshPrimitive* pPrimitive;
ExtensionExtMeshFeatures* pExtension;
END_DEFINE_SPEC(FCesiumPrimitiveFeaturesSpec)

void FCesiumPrimitiveFeaturesSpec::Define() {
  Describe("Constructor", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pExtension = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
    });

    It("constructs with no feature ID sets", [this]() {
      // This is technically disallowed by the spec, but just make sure it's
      // handled reasonably.
      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      TArray<FCesiumFeatureIdSet> featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
              primitiveFeatures);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 0);
    });

    It("constructs with single feature ID set", [this]() {
      ExtensionExtMeshFeaturesFeatureId& featureID =
          pExtension->featureIds.emplace_back();
      featureID.featureCount = 10;

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet>& featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
              primitiveFeatures);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 1);

      const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
      TestEqual(
          "Feature Count",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
          static_cast<int64>(featureID.featureCount));
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Implicit);
    });

    It("constructs with multiple feature ID sets", [this]() {
      const std::vector<uint8_t> attributeIDs{0, 0, 0};
      AddFeatureIDsAsAttributeToModel(model, *pPrimitive, attributeIDs, 1, 0);

      const std::vector<uint8_t> textureIDs{1, 2, 3};
      const std::vector<glm::vec2> texCoords{
          glm::vec2(0, 0),
          glm::vec2(0.45, 0),
          glm::vec2(0.86, 0)};
      AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          textureIDs,
          3,
          3,
          1,
          texCoords);

      ExtensionExtMeshFeaturesFeatureId& implicitIDs =
          pExtension->featureIds.emplace_back();
      implicitIDs.featureCount = 3;

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet>& featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
              primitiveFeatures);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 3);

      const std::vector<ECesiumFeatureIdType> expectedTypes{
          ECesiumFeatureIdType::Attribute,
          ECesiumFeatureIdType::Texture,
          ECesiumFeatureIdType::Implicit};

      for (size_t i = 0; i < featureIDSets.Num(); i++) {
        const FCesiumFeatureIdSet& featureIDSet =
            featureIDSets[static_cast<int32>(i)];
        const ExtensionExtMeshFeaturesFeatureId& gltfFeatureID =
            pExtension->featureIds[i];
        TestEqual(
            "Feature Count",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(featureIDSet),
            static_cast<int64>(gltfFeatureID.featureCount));
        TestEqual(
            "FeatureIDType",
            UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
            expectedTypes[i]);
      }
    });
  });

  Describe("GetFeatureIDSetsOfType", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pExtension = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();

      const std::vector<uint8_t> attributeIDs{0, 0, 0};
      AddFeatureIDsAsAttributeToModel(model, *pPrimitive, attributeIDs, 1, 0);

      const std::vector<uint8_t> textureIDs{1, 2, 3};
      const std::vector<glm::vec2> texCoords{
          glm::vec2(0, 0),
          glm::vec2(0.5, 0),
          glm::vec2(1, 0)};
      AddFeatureIDsAsTextureToModel(
          model,
          *pPrimitive,
          textureIDs,
          3,
          3,
          1,
          texCoords);

      ExtensionExtMeshFeaturesFeatureId& implicitIDs =
          pExtension->featureIds.emplace_back();
      implicitIDs.featureCount = 3;
    });

    It("gets feature ID attribute", [this]() {
      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet> featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
              primitiveFeatures,
              ECesiumFeatureIdType::Attribute);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 1);

      const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Attribute);

      const FCesiumFeatureIdAttribute& attribute =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
              featureIDSet);
      TestEqual(
          "AttributeStatus",
          UCesiumFeatureIdAttributeBlueprintLibrary::
              GetFeatureIDAttributeStatus(attribute),
          ECesiumFeatureIdAttributeStatus::Valid);
    });

    It("gets feature ID texture", [this]() {
      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet> featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
              primitiveFeatures,
              ECesiumFeatureIdType::Texture);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 1);

      const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Texture);

      const FCesiumFeatureIdTexture& texture =
          UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
              featureIDSet);
      TestEqual(
          "TextureStatus",
          UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
              texture),
          ECesiumFeatureIdTextureStatus::Valid);
    });

    It("gets implicit feature ID", [this]() {
      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const TArray<FCesiumFeatureIdSet> featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
              primitiveFeatures,
              ECesiumFeatureIdType::Implicit);
      TestEqual("Number of FeatureIDSets", featureIDSets.Num(), 1);

      const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
      TestEqual(
          "FeatureIDType",
          UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(featureIDSet),
          ECesiumFeatureIdType::Implicit);
    });
  });

  Describe("GetFirstVertexFromFace", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pExtension = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
    });

    It("returns -1 for out-of-bounds face index", [this]() {
      const std::vector<uint8_t> indices{0, 1, 2, 0, 2, 3};
      CreateIndicesForPrimitive(
          model,
          *pPrimitive,
          AccessorSpec::Type::SCALAR,
          AccessorSpec::ComponentType::UNSIGNED_BYTE,
          indices);

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
      TestEqual(
          "VertexIndexForNegativeFace",
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
              primitiveFeatures,
              -1),
          -1);
      TestEqual(
          "VertexIndexForOutOfBoundsFace",
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
              primitiveFeatures,
              2),
          -1);
    });

    It("returns correct value for primitive without indices", [this]() {
      Accessor& accessor = model.accessors.emplace_back();
      accessor.count = 9;
      const int64 numFaces = accessor.count / 3;

      pPrimitive->attributes.insert(
          {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
      for (int64 i = 0; i < numFaces; i++) {
        TestEqual(
            "VertexIndexForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
                primitiveFeatures,
                i),
            i * 3);
      }
    });

    It("returns correct value for primitive with indices", [this]() {
      const std::vector<uint8_t> indices{0, 1, 2, 0, 2, 3, 4, 5, 6};
      CreateIndicesForPrimitive(
          model,
          *pPrimitive,
          AccessorSpec::Type::SCALAR,
          AccessorSpec::ComponentType::UNSIGNED_BYTE,
          indices);

      Accessor& accessor = model.accessors.emplace_back();
      accessor.count = 7;
      pPrimitive->attributes.insert(
          {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);

      const size_t numFaces = indices.size() / 3;
      for (size_t i = 0; i < numFaces; i++) {
        TestEqual(
            "VertexIndexForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
                primitiveFeatures,
                static_cast<int64>(i)),
            indices[i * 3]);
      }
    });
  });

  Describe("GetFeatureIDFromFace", [this]() {
    BeforeEach([this]() {
      model = Model();
      Mesh& mesh = model.meshes.emplace_back();
      pPrimitive = &mesh.primitives.emplace_back();
      pExtension = &pPrimitive->addExtension<ExtensionExtMeshFeatures>();
    });

    It("returns -1 for primitive with empty feature ID sets", [this]() {
      const std::vector<uint8_t> indices{0, 1, 2, 0, 2, 3};
      CreateIndicesForPrimitive(
          model,
          *pPrimitive,
          AccessorSpec::Type::SCALAR,
          AccessorSpec::ComponentType::UNSIGNED_BYTE,
          indices);

      Accessor& accessor = model.accessors.emplace_back();
      accessor.count = 6;
      pPrimitive->attributes.insert(
          {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

      // Adds empty feature ID.
      pExtension->featureIds.emplace_back();

      FCesiumPrimitiveFeatures primitiveFeatures =
          FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
      const TArray<FCesiumFeatureIdSet>& featureIDSets =
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
              primitiveFeatures);

      TestEqual(
          "FeatureIDForOutOfBoundsFace",
          UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
              primitiveFeatures,
              featureIDSets[0],
              0),
          -1);
    });

    Describe("FeatureIDAttribute", [this]() {
      It("returns -1 for out-of-bounds face index", [this]() {
        std::vector<uint8_t> attributeIDs{1, 1, 1};
        AddFeatureIDsAsAttributeToModel(model, *pPrimitive, attributeIDs, 1, 0);

        const std::vector<uint8_t> indices{0, 1, 2};
        CreateIndicesForPrimitive(
            model,
            *pPrimitive,
            AccessorSpec::Type::SCALAR,
            AccessorSpec::ComponentType::UNSIGNED_BYTE,
            indices);

        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 3;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        TestEqual(
            "FeatureIDForNegativeFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                -1),
            -1);
        TestEqual(
            "FeatureIDForOutOfBoundsFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                2),
            -1);
      });

      It("returns correct values for primitive without indices", [this]() {
        std::vector<uint8_t> attributeIDs{1, 1, 1, 2, 2, 2, 0, 0, 0};
        AddFeatureIDsAsAttributeToModel(model, *pPrimitive, attributeIDs, 3, 0);

        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 9;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        const size_t numFaces = static_cast<size_t>(accessor.count / 3);
        for (size_t i = 0; i < numFaces; i++) {
          TestEqual(
              "FeatureIDForFace",
              UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                  primitiveFeatures,
                  featureIDSets[0],
                  static_cast<int64>(i)),
              attributeIDs[i * 3]);
        }
      });

      It("returns correct values for primitive with indices", [this]() {
        std::vector<uint8_t> attributeIDs{1, 1, 1, 1, 0, 0, 0};
        AddFeatureIDsAsAttributeToModel(model, *pPrimitive, attributeIDs, 2, 0);

        const std::vector<uint8_t> indices{0, 1, 2, 0, 2, 3, 4, 5, 6};
        CreateIndicesForPrimitive(
            model,
            *pPrimitive,
            AccessorSpec::Type::SCALAR,
            AccessorSpec::ComponentType::UNSIGNED_BYTE,
            indices);

        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 7;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        const size_t numFaces = indices.size() / 3;
        for (size_t i = 0; i < numFaces; i++) {
          TestEqual(
              "FeatureIDForFace",
              UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                  primitiveFeatures,
                  featureIDSets[0],
                  static_cast<int64>(i)),
              attributeIDs[i * 3]);
        }
      });
    });

    Describe("FeatureIDTexture", [this]() {
      It("returns -1 for out-of-bounds face index", [this]() {
        const std::vector<uint8_t> textureIDs{0};
        const std::vector<glm::vec2> texCoords{
            glm::vec2(0, 0),
            glm::vec2(0, 0),
            glm::vec2(0, 0)};
        AddFeatureIDsAsTextureToModel(
            model,
            *pPrimitive,
            textureIDs,
            4,
            4,
            1,
            texCoords);

        const std::vector<uint8_t> indices{0, 1, 2};
        CreateIndicesForPrimitive(
            model,
            *pPrimitive,
            AccessorSpec::Type::SCALAR,
            AccessorSpec::ComponentType::UNSIGNED_BYTE,
            indices);

        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 3;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        TestEqual(
            "FeatureIDForNegativeFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                -1),
            -1);
        TestEqual(
            "FeatureIDForOutOfBoundsFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                2),
            -1);
      });

      It("returns correct values for primitive without indices", [this]() {
        const std::vector<uint8_t> textureIDs{0, 1, 2, 3};
        const std::vector<glm::vec2> texCoords{
            glm::vec2(0, 0),
            glm::vec2(0, 0),
            glm::vec2(0, 0),
            glm::vec2(0.75, 0),
            glm::vec2(0.75, 0),
            glm::vec2(0.75, 0)};
        AddFeatureIDsAsTextureToModel(
            model,
            *pPrimitive,
            textureIDs,
            4,
            4,
            1,
            texCoords);

        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 6;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                0),
            0);
        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                1),
            3);
      });

      It("returns correct values for primitive with indices", [this]() {
        const std::vector<uint8_t> textureIDs{0, 1, 2, 3};
        const std::vector<glm::vec2> texCoords{
            glm::vec2(0, 0),
            glm::vec2(0.25, 0),
            glm::vec2(0.5, 0),
            glm::vec2(0.75, 0)};
        AddFeatureIDsAsTextureToModel(
            model,
            *pPrimitive,
            textureIDs,
            4,
            4,
            1,
            texCoords);

        const std::vector<uint8_t> indices{0, 1, 2, 2, 0, 3};
        CreateIndicesForPrimitive(
            model,
            *pPrimitive,
            AccessorSpec::Type::SCALAR,
            AccessorSpec::ComponentType::UNSIGNED_BYTE,
            indices);

        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 4;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                0),
            0);
        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                1),
            2);
      });
    });

    Describe("ImplicitFeatureIDs", [this]() {
      BeforeEach([this]() {
        ExtensionExtMeshFeaturesFeatureId& implicitIDs =
            pExtension->featureIds.emplace_back();
        implicitIDs.featureCount = 6;
      });

      It("returns -1 for out-of-bounds face index", [this]() {
        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 6;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        TestEqual(
            "FeatureIDForNegativeFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                -1),
            -1);
        TestEqual(
            "FeatureIDForOutOfBoundsFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                10),
            -1);
      });

      It("returns correct values for primitive without indices", [this]() {
        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 6;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                0),
            0);
        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                1),
            3);
      });

      It("returns correct values for primitive with indices", [this]() {
        const std::vector<uint8_t> indices{2, 1, 0, 3, 4, 5};
        CreateIndicesForPrimitive(
            model,
            *pPrimitive,
            AccessorSpec::Type::SCALAR,
            AccessorSpec::ComponentType::UNSIGNED_BYTE,
            indices);

        Accessor& accessor = model.accessors.emplace_back();
        accessor.count = 4;
        pPrimitive->attributes.insert(
            {"POSITION", static_cast<int32_t>(model.accessors.size() - 1)});

        FCesiumPrimitiveFeatures primitiveFeatures =
            FCesiumPrimitiveFeatures(model, *pPrimitive, *pExtension);
        const TArray<FCesiumFeatureIdSet>& featureIDSets =
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
                primitiveFeatures);

        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                0),
            2);
        TestEqual(
            "FeatureIDForFace",
            UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
                primitiveFeatures,
                featureIDSets[0],
                1),
            3);
      });
    });
  });
}
