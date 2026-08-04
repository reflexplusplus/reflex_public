//here is some psuedo code from chatgpt
//this is maybe a solution how we could use multiple framebuffers with different AA factors
//to have dynamic toggleable AA per Graphic




#include <GL/glew.h>
#include <vector>

enum class MSAA_Level { None = 1, MSAA_2X = 2, MSAA_4X = 4, MSAA_8X = 8 };

class Canvas {
public:
    GLuint fbo1x, fbo2x, fbo4x, fbo8x;
    GLuint colorBuffer2x, colorBuffer4x, colorBuffer8x;
    GLuint depthBuffer2x, depthBuffer4x, depthBuffer8x;
    int width, height;

    Canvas(int w, int h) : width(w), height(h) {
        createFramebuffers();
    }

    ~Canvas() {
        glDeleteFramebuffers(1, &fbo1x);
        glDeleteFramebuffers(1, &fbo2x);
        glDeleteFramebuffers(1, &fbo4x);
        glDeleteFramebuffers(1, &fbo8x);
    }

    void createMSAAFramebuffer(GLuint &fbo, GLuint &colorBuffer, GLuint &depthBuffer, int samples) {
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);

        glGenRenderbuffers(1, &colorBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, colorBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_RGBA8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffer);

        glGenRenderbuffers(1, &depthBuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            printf("MSAA Framebuffer (%dx) is incomplete!\n", samples);
        }
    }

    void createFramebuffers() {
        // Default FBO (No MSAA)
        glGenFramebuffers(1, &fbo1x);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo1x);

        // Create MSAA Framebuffers
        createMSAAFramebuffer(fbo2x, colorBuffer2x, depthBuffer2x, 2);
        createMSAAFramebuffer(fbo4x, colorBuffer4x, depthBuffer4x, 4);
        createMSAAFramebuffer(fbo8x, colorBuffer8x, depthBuffer8x, 8);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bindFramebuffer(MSAA_Level level) {
        switch (level) {
            case MSAA_Level::None: glBindFramebuffer(GL_FRAMEBUFFER, fbo1x); break;
            case MSAA_Level::MSAA_2X: glBindFramebuffer(GL_FRAMEBUFFER, fbo2x); break;
            case MSAA_Level::MSAA_4X: glBindFramebuffer(GL_FRAMEBUFFER, fbo4x); break;
            case MSAA_Level::MSAA_8X: glBindFramebuffer(GL_FRAMEBUFFER, fbo8x); break;
        }
    }

    void resolveFramebuffer(MSAA_Level level) {
        if (level == MSAA_Level::None) return;

        GLuint srcFBO = (level == MSAA_Level::MSAA_2X) ? fbo2x :
                        (level == MSAA_Level::MSAA_4X) ? fbo4x : fbo8x;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }
};





struct Vertex {
    float x, y;
};

class Graphic {
public:
    GLuint vao, vbo;
    std::vector<Vertex> vertices;
    MSAA_Level msaaLevel;

    Graphic(const std::vector<Vertex>& verts, MSAA_Level msaa)
        : vertices(verts), msaaLevel(msaa) {
        setupVBO();
    }

    ~Graphic() {
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    void setupVBO() {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

        glBindVertexArray(0);
    }

    void Render(Canvas& canvas) {
        canvas.bindFramebuffer(msaaLevel);
        
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        glBindVertexArray(0);

        canvas.resolveFramebuffer(msaaLevel);
    }
};
