#include "vboMesh.h"
#include "platform.h"
#include <cstring>

#define MAX_INDEX_VALUE 65535 // Maximum value of GLushort

int VboMesh::s_validGeneration = 0;

VboMesh::VboMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode) : m_vertexLayout(_vertexLayout) {

    m_glVertexBuffer = 0;
    m_glIndexBuffer = 0;
    m_nVertices = 0;
    m_nIndices = 0;

    m_isUploaded = false;

    setDrawMode(_drawMode);
}

VboMesh::VboMesh() {
    m_glVertexBuffer = 0;
    m_glIndexBuffer = 0;
    m_nVertices = 0;
    m_nIndices = 0;

    m_isUploaded = false;
}

VboMesh::~VboMesh() {
    if (m_glVertexBuffer) glDeleteBuffers(1, &m_glVertexBuffer);
    if (m_glIndexBuffer) glDeleteBuffers(1, &m_glIndexBuffer);

    m_vertexData.clear();
    m_indices.clear();

    for (auto p : m_newVertices)
      delete[] p.first;

    for (auto p : m_newIndices)
      delete[] p.first;
}

void VboMesh::setVertexLayout(std::shared_ptr<VertexLayout> _vertexLayout) {
    m_vertexLayout = _vertexLayout;
}

void VboMesh::setDrawMode(GLenum _drawMode) {
    switch (_drawMode) {
        case GL_POINTS:
        case GL_LINE_STRIP:
        case GL_LINE_LOOP:
        case GL_LINES:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_FAN:
        case GL_TRIANGLES:
            m_drawMode = _drawMode;
            break;
        default:
            logMsg("WARNING: Invalid draw mode for mesh! Defaulting to GL_TRIANGLES\n");
            m_drawMode = GL_TRIANGLES;
    }
}

void VboMesh::addVertex(GLbyte* _vertex) {

    addVertices(_vertex, 1);

}

void VboMesh::addVertices(GLbyte* _vertices, int _nVertices) {

    if (m_isUploaded) {
        logMsg("ERROR: VboMesh cannot add vertices after upload!\n");
        return;
    }
    
    // Only add up to 65535 vertices, any more will overflow our 16-bit indices
    int indexSpace = MAX_INDEX_VALUE - m_nVertices;
    
    if (_nVertices > MAX_INDEX_VALUE) {
        logMsg("WARNING: Cannot add > %d vertices in one call, truncating mesh\n", MAX_INDEX_VALUE);
        _nVertices = indexSpace;
    }
    
    if (_nVertices > indexSpace || m_backupMesh) {
        if (!m_backupMesh) {
            m_backupMesh.reset(new VboMesh(m_vertexLayout, m_drawMode));
        }
        m_backupMesh->addVertices(_vertices, _nVertices);
        return;
    }

    int vertexBytes = m_vertexLayout->getStride() * _nVertices;
    auto vertices = new GLubyte[vertexBytes];
    std::memcpy(vertices, _vertices, vertexBytes);

    m_newVertices.emplace_back(vertices, vertexBytes);
    m_nVertices += _nVertices;
}

int VboMesh::numVertices() const {
    if (m_backupMesh) {
        return m_nVertices + m_backupMesh->numVertices();
    }
    return m_nVertices;
}

void VboMesh::addIndex(int* _index) {

    addIndices(_index, 1);

}

void VboMesh::addIndices(int* _indices, int _nIndices) {
    
    if (m_isUploaded) {
        logMsg("ERROR: VboMesh cannot add indices after upload!\n");
        return;
    }
    
    if (m_backupMesh) {
        for (int i = 0; i < _nIndices; i++) {
            _indices[i] -= m_nVertices;
        }
        m_backupMesh->addIndices(_indices, _nIndices);
        return;
    }

    auto indices = new GLushort[_nIndices];
    for (int i = 0; i < _nIndices; i++) indices[i] = _indices[i];

    m_newIndices.emplace_back(indices, _nIndices);
    m_nIndices += _nIndices;
}

int VboMesh::numIndices() const {
    if (m_backupMesh) {
        return m_nIndices + m_backupMesh->numIndices();
    }
    return m_nIndices;
}

void VboMesh::upload() {
    if (m_nVertices > 0) {
        // Generate vertex buffer, if needed
        if (m_glVertexBuffer == 0) glGenBuffers(1, &m_glVertexBuffer);

        int vertexBytes = m_vertexLayout->getStride() * m_nVertices;

        auto vertices = new GLubyte[vertexBytes];
        int offset = 0;

        for (auto p : m_newVertices) {
            std::memcpy(vertices + offset, p.first, p.second);
            offset += p.second;
            delete[] p.first;
        }
        m_newVertices.clear();

        // Buffer vertex data
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertexBytes, vertices, GL_STATIC_DRAW);
        delete[] vertices;
    }

    if (m_nIndices > 0) {
        if (m_glIndexBuffer == 0) glGenBuffers(1, &m_glIndexBuffer);

        auto indices = new GLushort[m_nIndices];

        int offset = 0;
        for (auto p : m_newIndices) {
            std::memcpy(indices + offset, p.first, p.second * sizeof(GLushort));
            offset += p.second;
            delete[] p.first;
        }
        m_newIndices.clear();

        // Buffer element index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_nIndices * sizeof(GLushort),
                     indices, GL_STATIC_DRAW);
        delete[] indices;
    }

    //m_vertexData.clear();
    //m_indices.clear();
    // TODO: For now, we retain copies of the vertex and index data in CPU memory to allow VBOs
    // to easily rebuild themselves after GL context loss. For optimizing memory usage (and for
    // other reasons) we'll want to change this in the future. This probably means going back to
    // data sources and styles to rebuild the vertex data.
    
    if (m_backupMesh) {
        m_backupMesh->upload();
    }
    
    m_generation = s_validGeneration;

    m_isUploaded = true;

}

void VboMesh::draw(const std::shared_ptr<ShaderProgram> _shader) {

    checkValidity();
    if (m_nVertices == 0 && m_nIndices == 0) return;

    // Ensure that geometry is buffered into GPU
    if (!m_isUploaded) {
        upload();
    }
    
    // Bind buffers for drawing
    if (m_nVertices > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, m_glVertexBuffer);
    }

    if (m_nIndices > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
    }

    // Enable shader program
    _shader->use();

    // Enable vertex attribs via vertex layout object
    m_vertexLayout->enable(_shader);

    // Draw as elements or arrays
    if (m_nIndices > 0) {
        glDrawElements(m_drawMode, m_nIndices, GL_UNSIGNED_SHORT, 0);
    } else if (m_nVertices > 0) {
        glDrawArrays(m_drawMode, 0, m_nVertices);
    }
    
    if (m_backupMesh) {
        m_backupMesh->draw(_shader);
    }

}

void VboMesh::checkValidity() {
    if (m_generation != s_validGeneration) {
        m_isUploaded = false;
        m_glVertexBuffer = 0;
        m_glIndexBuffer = 0;
        
        m_generation = s_validGeneration;
    }
}

void VboMesh::invalidateAllVBOs() {
    
    ++s_validGeneration;
    
}
