#include "Core.h"
#include "FObjImporter.h"

#include <fstream>
#include <sstream>
#include <filesystem>

bool FObjImporter::ParseObj(const FString& FilePath, FObjInfo& OutObjInfo)
{
    std::ifstream File(FilePath.CStr());

    if (!File.is_open())
    {
        return false;
    }

    int32 CurrentMaterialIndex = -1;

    OutObjInfo.SourcePath = FilePath;
    std::string Line;

    while (std::getline(File, Line))
    {
        std::istringstream Stream(Line);
        std::string Command;
        Stream >> Command;

        if (Command == "v")
        {
            float X, Y, Z;
            Stream >> X >> Y >> Z;
            OutObjInfo.Positions.Add(FVector(X, Y, Z));
        }
        else if (Command == "vt")
        {
            float U, V;
            Stream >> U >> V;
            OutObjInfo.UVs.Add(FVector2(U, V));
        }
        else if (Command == "vn")
        {
            float X, Y, Z;
            Stream >> X >> Y >> Z;
            OutObjInfo.Normals.Add(FVector(X, Y, Z));
        }
        else if (Command == "f")
        {
            FObjFace Face;
            Face.MaterialIndex = CurrentMaterialIndex;

            std::string Token;
            while (Stream >> Token)
            {
                FObjVertexIndex VertexIndex;

                if (!ParseFaceVertex(Token, OutObjInfo, VertexIndex))
                {
                    return false;
                }

                Face.Vertices.Add(VertexIndex);
            }

            if (Face.Vertices.Num() < 3)
            {
                return false;
            }
            OutObjInfo.Faces.Add(Face);
        }
        else if (Command == "usemtl")
        {
            std::string MaterialName;

            Stream >> MaterialName;

            for (int32 MaterialIndex = 0; MaterialIndex < OutObjInfo.Materials.Num(); ++MaterialIndex)
            {
                if (OutObjInfo.Materials[MaterialIndex].Name == FString(MaterialName))
                {
                    CurrentMaterialIndex = MaterialIndex;

                    break;
                }
            }
        }
        else if (Command == "mtllib")
        {
            std::string MtlFileName;
            Stream >> MtlFileName;

            const std::filesystem::path ObjPath(FilePath.CStr());
            const std::filesystem::path MtlPath = ObjPath.parent_path() / MtlFileName;

            ParseMtl(FString(MtlPath.string()), OutObjInfo);
        }
    }

    return true;
}

bool FObjImporter::ConvertToMeshDescription(const FObjInfo& ObjInfo, FMeshDescription& OutMeshDescription)
{
    OutMeshDescription.Vertices.Empty();
    OutMeshDescription.VertexInstances.Empty();
    OutMeshDescription.Triangles.Empty();
    OutMeshDescription.PolygonGroups.Empty();

    FPolygonGroup DefaultPolygonGroup;
    DefaultPolygonGroup.MaterialName = FString("DefaultMaterial");
    OutMeshDescription.PolygonGroups.Add(DefaultPolygonGroup);

    for (const FObjMaterialInfo& ObjMaterial : ObjInfo.Materials)
    {
        FPolygonGroup PolygonGroup;
        PolygonGroup.MaterialName = ObjMaterial.Name;
        OutMeshDescription.PolygonGroups.Add(PolygonGroup);
    }

    for (const FVector& Position : ObjInfo.Positions)
    {
        FMeshVertexPosition MeshVertex;
        MeshVertex.Position = Position;

        OutMeshDescription.Vertices.Add(MeshVertex);
    }

    for (const FObjFace& Face : ObjInfo.Faces)
    {
        const int32 FaceVertexCount = Face.Vertices.Num();
        if (FaceVertexCount < 3)
        {
            return false;
        }

        TArray<FVertexInstanceID> FaceInstanceIDs;

        for (const FObjVertexIndex& ObjIndex : Face.Vertices)
        {
            if (ObjIndex.PositionIndex < 0 || ObjIndex.PositionIndex >= ObjInfo.Positions.Num())
            {
                return false;
            }
            FMeshVertexInstance VertexInstance;
            VertexInstance.VertexID = static_cast<FVertexID>(ObjIndex.PositionIndex);

            if (ObjIndex.UVIndex >= 0)
            {
                if (ObjIndex.UVIndex >= ObjInfo.UVs.Num())
                {
                    return false;
                }

                VertexInstance.TexCoord = ObjInfo.UVs[ObjIndex.UVIndex];
            }

            if (ObjIndex.NormalIndex >= 0)
            {
                if (ObjIndex.NormalIndex >= ObjInfo.Normals.Num())
                {
                    return false;
                }

                VertexInstance.Normal = ObjInfo.Normals[ObjIndex.NormalIndex];
            }

            VertexInstance.Color = FVector4(1, 1, 1, 1);

            const FVertexInstanceID NewInstanceID = OutMeshDescription.VertexInstances.Add(VertexInstance);
            FaceInstanceIDs.Add(NewInstanceID);
        }

        for (int32 TriangleIndex = 1; TriangleIndex < FaceVertexCount - 1; ++TriangleIndex)
        {
            FMeshTriangle Triangle;

            Triangle.Corners[0] = FaceInstanceIDs[0];
            Triangle.Corners[1] = FaceInstanceIDs[TriangleIndex];
            Triangle.Corners[2] = FaceInstanceIDs[TriangleIndex + 1];

            Triangle.PolygonGroupID = Face.MaterialIndex >= 0
                ? static_cast<FPolygonGroupID>(Face.MaterialIndex + 1)
                : 0;

            OutMeshDescription.Triangles.Add(Triangle);
        }
    }
    return true;
}


bool FObjImporter::ParseFaceVertex(const FString& Token, const FObjInfo& ObjInfo, FObjVertexIndex& OutIndex) const
{
    int32 RawPositionIndex = -1;
    int32 RawUVIndex = -1;
    int32 RawNormalIndex = -1;

    const int32 ParseCount = std::sscanf(Token.CStr(), "%d/%d/%d",
        &RawPositionIndex, &RawUVIndex, &RawNormalIndex);

    if (ParseCount != 3)
    {
        return false;
    }
    
    OutIndex.PositionIndex = ResolveObjIndex(RawPositionIndex, ObjInfo.Positions.Num());
    OutIndex.UVIndex = ResolveObjIndex(RawUVIndex, ObjInfo.UVs.Num());
    OutIndex.NormalIndex = ResolveObjIndex(RawNormalIndex, ObjInfo.Normals.Num());

    return true;
}


bool FObjImporter::ParseMtl(const FString& FilePath, FObjInfo& OutObjInfo)
{
    std::ifstream File(FilePath.CStr());

    if (!File.is_open())
    {
        return false;
    }

    FObjMaterialInfo* CurrentMaterial = nullptr;
    std::string Line;

    while (std::getline(File, Line))
    {
        std::istringstream Stream(Line);
        std::string Command;
        Stream >> Command;

        if (Command.empty())
        {
            continue;
        }

        if (Command[0] == '#')
        {
            continue;
        }

        if (Command == "newmtl")
        {
            std::string MaterialName;
            Stream >> MaterialName;
            if (MaterialName.empty())
            {
                CurrentMaterial = nullptr;

                continue;
            }

            FObjMaterialInfo NewMaterial;
            NewMaterial.Name = FString(MaterialName);
            OutObjInfo.Materials.Add(NewMaterial);

            CurrentMaterial = &OutObjInfo.Materials[OutObjInfo.Materials.Num() - 1];
            continue;
        }
        if (!CurrentMaterial)
        {
            continue;
        }

        if (Command == "Kd")
        {
            float R = 1.0f;
            float G = 1.0f;
            float B = 1.0f;
            if (Stream >> R >> G >> B)
            {
                CurrentMaterial->DiffuseColor = FVector(R, G, B);
            }
        }
        else if (Command == "d")
        {
            float Opacity = 1.0f;
            if (Stream >> Opacity)
            {
                CurrentMaterial->Opacity = Opacity;
            }
        }
        else if (Command == "Tr")
        {
            float Transparency = 0.0f;
            if (Stream >> Transparency)
            {
                CurrentMaterial->Opacity = 1.0f - Transparency;
            }
        }
        else if (Command == "map_Kd")
        {
            std::string TexturePath;
            Stream >> TexturePath;
            CurrentMaterial->DiffuseTexturePath = FString(TexturePath);
        }
    }

    return true;
}

int32 FObjImporter::ResolveObjIndex(int32 ObjIndex, int32 ElementCount) const
{
    if (ObjIndex > 0) 
    {
        return ObjIndex - 1;
    }

    if (ObjIndex < 0)
    {
        return ElementCount + ObjIndex;
    }

    return -1;
}
