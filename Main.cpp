#include "Framework.h"
#include "Shader.h"

#define MODEL_PATH "models/dragon.dae"

// Note: See the SMFL documentation for info on setting up fullscreen mode
// and using rendering settings
// http://www.sfml-dev.org/tutorials/1.6/window-window.php
sf::WindowSettings settings(24, 8, 2);
sf::Window window(sf::VideoMode(800, 600), "CS248 Rules!", sf::Style::Close, settings);
 
// This is a clock you can use to control animation.  For more info, see:
// http://www.sfml-dev.org/tutorials/1.6/window-time.php
sf::Clock clck;

// This creates an asset importer using the Open Asset Import library.
// It automatically manages resources for you, and frees them when the program
// exits.
Assimp::Importer importer;
const aiScene* scene;
const aiMesh* mesh;
std::vector<unsigned> indexBuffer;

// Vertex shader
std::auto_ptr<Shader> shader;

// Texture
std::auto_ptr<sf::Image> diffuseMap;
std::auto_ptr<sf::Image> specularMap;

void initOpenGL();
void loadAssets();
void handleInput();
void renderFrame();

int main(int argc, char** argv) {

    initOpenGL();
    loadAssets();

    // Put your game loop here (i.e., render with OpenGL, update animation)
    while (window.IsOpened()) {

        handleInput();
        renderFrame();
        window.Display();
    }

    return 0;
}




void initOpenGL() {
    // Initialize GLEW on Windows, to make sure that OpenGL 2.0 is loaded
#ifdef FRAMEWORK_USE_GLEW
    GLint error = glewInit();
    if (GLEW_OK != error) {
        std::cerr << glewGetErrorString(error) << std::endl;
        exit(-1);
    }
    if (!GLEW_VERSION_2_0) {
        std::cerr << "This program requires OpenGL 2.0" << std::endl;
        exit(-1);
    }
#endif

    // This initializes OpenGL with some common defaults.  More info here:
    // http://www.sfml-dev.org/tutorials/1.6/window-opengl.php
    glClearDepth(1.0f);
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
    glEnable(GL_DEPTH_TEST);
}




void loadAssets() {
    // Read in an asset file, and do some post-processing.  There is much 
    // more you can do with this asset loader, including load textures.
    // More info is here:
    // http://assimp.sourceforge.net/lib_html/usage.html
    scene = importer.ReadFile(MODEL_PATH,  
        //aiProcess_CalcTangentSpace |
        aiProcess_Triangulate |
        //aiProcess_JoinIdenticalVertices |
        aiProcessPreset_TargetRealtime_Quality);

    if (!scene || scene->mNumMeshes <= 0) {
        std::cerr << importer.GetErrorString() << std::endl;
        exit(-1);
    }
    
    // Just render the first mesh in the imported scene file
    mesh = scene->mMeshes[0];
	
    // Set up the index buffer.  Each face should have 3 vertices since we
    // specified aiProcess_Triangulate
    indexBuffer.reserve(mesh->mNumFaces * 3);
    for (unsigned i = 0; i < mesh->mNumFaces; i++) {
        for (unsigned j = 0; j < mesh->mFaces[i].mNumIndices; j++) {
            indexBuffer.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }
	
    // Load the vertex shader
    shader.reset(new Shader("shaders/phong"));
	if (!shader->loaded()) {
		std::cerr << "Shader failed to load" << std::endl;
		std::cerr << shader->errors() << std::endl;
		exit(-1);
	}

    // Load the textures
    diffuseMap.reset(new sf::Image());
    diffuseMap->LoadFromFile("models/dragon-diffuse.jpg");
    specularMap.reset(new sf::Image());
    specularMap->LoadFromFile("models/dragon-specular.jpg");
}




void handleInput() {
    // Event loop, for processing user input, etc.  For more info, see:
    // http://www.sfml-dev.org/tutorials/1.6/window-events.php
    sf::Event evt;
    while (window.GetEvent(evt)) {
        switch (evt.Type) {
        case sf::Event::Closed: 
            // Close the window.  This will cause the game loop to exit,
            // because the IsOpened() function will no longer return true.
            window.Close(); 
            break;
        case sf::Event::Resized: 
            // If the window is resized, then we need to change the perspective
            // transformation and viewport
            glViewport(0, 0, evt.Size.Width, evt.Size.Height);
            break;
        }
    }
}



void setMatrices() {
    // Set up the projection and model-view matrices
    GLfloat aspectRatio = (GLfloat)window.GetWidth()/window.GetHeight();
    GLfloat nearClip = 0.1f;
    GLfloat farClip = 500.0f;
    GLfloat fieldOfView = 45.0f; // Degrees

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fieldOfView, aspectRatio, nearClip, farClip);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0f, 2.0f, -12.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    // Add a little rotation, using the elapsed time for smooth animation
    static float elapsed = 0.0f;
    elapsed += clck.GetElapsedTime();
    clck.Reset();
    glRotatef(20*elapsed, 0, 1, 0);
}



void setMaterial() {
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    aiColor3D color;

    // Get a handle to the diffuse, specular, and ambient variables
    // inside the shader.  Then set them with the diffuse, specular, and
    // ambient color.
    GLint diffuse = glGetUniformLocation(shader->programID(), "Kd");
    material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    glUniform3f(diffuse, color.r, color.g, color.b);

    // Specular material
    GLint specular = glGetUniformLocation(shader->programID(), "Ks");
    material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    glUniform3f(specular, color.r, color.g, color.b);
  
    // Ambient material
    GLint ambient = glGetUniformLocation(shader->programID(), "Ka");
    material->Get(AI_MATKEY_COLOR_DIFFUSE, color);
    glUniform3f(ambient, color.r, color.g, color.b);

    // Specular power
    GLint shininess = glGetUniformLocation(shader->programID(), "alpha");
    float value;
    if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, value)) {
        glUniform1f(shininess, value);
    } else {
        glUniform1f(shininess, 1);
    }
}



void setTextures() {
    // Get a "handle" to the texture variables inside our shader.  Then 
    // pass two textures to the shader: one for diffuse, and the other for
    // transparency.
    GLint diffuse = glGetUniformLocation(shader->programID(), "diffuseMap");
    glUniform1i(diffuse, 0); // The diffuse map will be GL_TEXTURE0
    glActiveTexture(GL_TEXTURE0);
    diffuseMap->Bind();
    
    // Transparency
    GLint specular = glGetUniformLocation(shader->programID(), "specularMap");
    glUniform1i(specular, 1); // The transparency map will be GL_TEXTURE1
    glActiveTexture(GL_TEXTURE1);
    specularMap->Bind();
}




void setMeshData() {
    // Get a handle to the variables for the vertex data inside the shader.
    GLint position = glGetAttribLocation(shader->programID(), "positionIn");
    glEnableVertexAttribArray(position);
    glVertexAttribPointer(position, 3, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mVertices);

    // Texture coords.  Note the [0] at the end, very important
    GLint texcoord = glGetAttribLocation(shader->programID(), "texcoordIn");
    glEnableVertexAttribArray(texcoord);
    glVertexAttribPointer(texcoord, 2, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mTextureCoords[0]);

    // Normals
    GLint normal = glGetAttribLocation(shader->programID(), "normalIn");
    glEnableVertexAttribArray(normal);
    glVertexAttribPointer(normal, 3, GL_FLOAT, 0, sizeof(aiVector3D), mesh->mNormals);
}

    

void renderFrame() {


    // Always clear the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(shader->programID());

    setMatrices();
    setMaterial();
    setTextures();
    setMeshData();

    // Draw the mesh
    glDrawElements(GL_TRIANGLES,  3*mesh->mNumFaces, GL_UNSIGNED_INT, &indexBuffer[0]);
}
