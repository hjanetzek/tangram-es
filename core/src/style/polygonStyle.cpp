#include "polygonStyle.h"
#include "util/builders.h"
#include "roadLayers.h"

PolygonStyle::PolygonStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
    m_material.setEmissionEnabled(false);
    m_material.setAmbientEnabled(true);
    m_material.setDiffuse(glm::vec4(1.0));
    m_material.setSpecularEnabled(true);
    
    constructVertexLayout();
    constructShaderProgram();
}

void PolygonStyle::constructVertexLayout() {
    
    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_texcoord", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_layer", 1, GL_FLOAT, false, 0}
    }));
    
}

void PolygonStyle::constructShaderProgram() {
    
    std::string vertShaderSrcStr = stringFromResource("polygon.vs");
    std::string fragShaderSrcStr = stringFromResource("polygon.fs");
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    m_material.injectOnProgram(m_shaderProgram); // This is a must for lighting !!
}

void PolygonStyle::buildPoint(Point& _point, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    // No-op
}

void PolygonStyle::buildLine(Line& _line, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosNormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoords;
    PolyLineOutput output = { points, indices, Builders::NO_SCALING_VECS, texcoords };
    
    GLuint abgr = 0xff969696; // Default road color
    
    Builders::buildPolyLine(_line, PolyLineOptions(), output);
    
    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec2 u = texcoords[i];
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y, abgr });
    }
    
    // Make sure indices get correctly offset
    int vertOffset = _mesh.numVertices();
    for (auto& ind : indices) {
        ind += vertOffset;
    }
    
    _mesh.addVertices((GLbyte*)vertices.data(), (int)vertices.size());
    _mesh.addIndices(indices.data(), (int)indices.size());
}

void PolygonStyle::buildPolygon(Polygon& _polygon, std::string& _layer, Properties& _props, VboMesh& _mesh) const {
    
    std::vector<PosNormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;
    PolygonOutput output = { points, indices, normals, texcoords };
    
    GLuint abgr = 0xffaaaaaa; // Default color
    GLfloat layer = 0;
    
    if (_layer == "buildings") {
        layer = ROAD_LAYER_OFFSET + 4;
        abgr = 0xffe6f0f2;
    } else if (_layer == "water") {
        layer = 2;
        abgr = 0xff917d1a;
    } else if (_layer == "roads") {
        layer = 3;
        abgr = 0xff969696;
    } else if (_layer == "earth") {
        layer = 0;
        abgr = 0xffa9b9c2;
    } else if (_layer == "landuse") {
        layer = 1;
        abgr = 0xff669171;
    }
    
    float height = _props.numericProps["height"]; // Inits to zero if not present in data
    float minHeight = _props.numericProps["min_height"]; // Inits to zero if not present in data
    
    if (minHeight != height) {
        for (auto& line : _polygon) {
            for (auto& point : line) {
                point.z = height;
            }
        }
        Builders::buildPolygonExtrusion(_polygon, minHeight, output);
    }
    
    Builders::buildPolygon(_polygon, output);
    
    for (size_t i = 0; i < points.size(); i++) {
        glm::vec3 p = points[i];
        glm::vec3 n = normals[i];
        glm::vec2 u = texcoords[i];
        vertices.push_back({ p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y, abgr, layer });
    }
    
    // Outlines for water polygons
    /*
    if (_layer == "water") {
        abgr = 0xfff2cc6c;
        size_t outlineStart = points.size();
        PolyLineOutput lineOutput = { points, indices, Builders::NO_SCALING_VECS, texcoords };
        PolyLineOptions outlineOptions = { CapTypes::ROUND, JoinTypes::ROUND, 0.02f };
        Builders::buildOutline(_polygon[0], outlineOptions, lineOutput);
        glm::vec3 normal(0.f, 0.f, 1.f); // The outline builder doesn't produce normals, so we'll add those now
        normals.insert(normals.end(), points.size() - normals.size(), normal);
        for (size_t i = outlineStart; i < points.size(); i++) {
            glm::vec3& p = points[i];
            glm::vec3& n = normals[i];
            glm::vec2& u = texcoords[i];
            vertices.push_back({ p.x, p.y, p.z + .02f, n.x, n.y, n.z, u.x, u.y, abgr });
        }
    }
    */
    
    // Make sure indices get correctly offset
    int vertOffset = _mesh.numVertices();
    for (auto& ind : indices) {
        ind += vertOffset;
    }
    
    _mesh.addVertices((GLbyte*)vertices.data(), (int)vertices.size());
    _mesh.addIndices(indices.data(), (int)indices.size());
}

// struct Key {
//     glm::vec3 p, n;
//     Key(glm::vec3 p, glm::vec3 n) : p(p), n(n) {}
//     bool operator==(const Key& other) const {
//         return (p == other.p && n == other.n);
//     }
// };

// namespace std {
//     template <>
//     struct hash<Key> {
//         std::size_t operator()(const Key& k) const {
//           // needs some shifting >>
//             return (std::hash<float>()(k.n.x) ^ std::hash<float>()(k.n.y) ^
//                     std::hash<float>()(k.n.z) ^ std::hash<float>()(k.p.x) ^
//                     std::hash<float>()(k.p.y) ^ std::hash<float>()(k.p.z));
//         }
//     };
// }

// static void addVertex(glm::vec3 p, glm::vec3 n, GLuint abgr, float layer,
//                       std::unordered_map<Key, uint32_t>& vertexMap,
//                       std::vector<uint32_t>& indices,
//                       std::vector<PolygonStyle::PosNormColVertex>& vertices) {
//     auto key = Key(p, n);
//     // wont reuse id == 0 this way
//     auto& id = vertexMap[key];
//     if (!id) {
//         logMsg("add id %d %f %f %f\n", id, p.x,p.y,p.z);
//         id = vertices.size();
//         vertices.push_back({p.x, p.y, p.z, n.x, n.y, n.z, 0, 0, abgr, layer});
//     }
//     indices.push_back(id);
// }

void PolygonStyle::addVertex(glm::vec3 p, glm::vec3 n, GLuint abgr, float layer,
                   std::vector<int>& indices,
                   std::vector<PosNormColVertex>& vertices) const {
  auto id = vertices.size();
  vertices.push_back({p.x, p.y, p.z, n.x, n.y, n.z, 0, 0, abgr, layer});
  indices.push_back(id);
}

void PolygonStyle::buildMesh(std::vector<uint32_t>& indices,
                             std::vector<Point>& points, std::string& _layer,
                             Properties& props, VboMesh& _mesh) const {
    GLuint abgr = 0xff969696;
    std::vector<PosNormColVertex> vertices;
    GLfloat layer = 1;

    //std::unordered_map<Key, uint32_t> vertexMap;
    std::vector<int> newIndices;

    for (size_t i = 0; i < indices.size(); i += 3) {
        auto p1 = points[indices[i + 0]];
        auto p2 = points[indices[i + 1]];
        auto p3 = points[indices[i + 2]];

        auto a = p2 - p1;
        auto b = p3 - p1;

        auto c = glm::cross(a, b);
        auto n = glm::vec3(0,0,1) - glm::normalize(c);

        addVertex(p1, n, abgr, layer, newIndices, vertices);
        addVertex(p3, n, abgr, layer, newIndices, vertices);
        addVertex(p2, n, abgr, layer, newIndices, vertices);
    }

    int vertOffset = _mesh.numVertices();
    for (auto& ind : newIndices) {
        ind += vertOffset;
    }

    _mesh.addVertices((GLbyte*)vertices.data(), (int)vertices.size());
    _mesh.addIndices(newIndices.data(), (int)newIndices.size());

    // for (size_t i = 0; i < points.size(); i++) {
    //     glm::vec3 p = points[i];
    //     glm::vec3 n(0);  // = normals[i];
    //     glm::vec2 u(0);  // = texcoords[i];
    //     vertices.push_back(
    //         {p.x, p.y, p.z, n.x, n.y, n.z, u.x, u.y, abgr, layer});
    // }

    //_mesh.addVertices((GLbyte*)vertices.data(), (int)vertices.size());
    //_mesh.addIndices(indices.data(), (int)indices.size());
}
