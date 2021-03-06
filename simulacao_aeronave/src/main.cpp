//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 4
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <iostream>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <math.h>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void Anda();
glm::vec4 curva_bezier(int which_cow);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    void*        first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    int          num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};
// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Zvel_maxima = 1;
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variaveis look_at
bool Look_at =true;
int primeiro=0;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;

float rotationX = 0.00f;
int rotateR =0;
int rotateL =0;
glm::vec4 camera_position_c=glm::vec4(0.0f,0.0f,0.0f,1.0f);;
bool afterRotation=false;
glm::vec4 u;
glm::vec4 w;

// Variáveis de movimentação da nave.
int anda_cima = 0;
int anda_direita = 0;
int anda_esquerda = 0;
float acelera_frente = 0.00000f;
int acelerando = 0;                     // indica se a nave está acelerando no momento
int freando = 0;
int vel_maxima = 0;

// Tiro da nave
float atira=0;
std::vector <std::pair<glm::mat4,glm::vec4>> tiros;
double tprev=glfwGetTime();
double tnow;
double deltat;
int last_i = -1;

// Constantes de movimentação da nave.
#define ROTATELIMIT 25                // limite de rotação da nave quando ela anda para a direita ou esquerda
#define RV 10;
#define RVB 20;

/* Valor de Parâmetro da Curva Bezier */
float valor_param_vaca1 = 0.0f;
float valor_param_vaca2 = 0.5f;

// Constante PI
#define PI 3.141592f

//Ponto do tiro;
std::vector<glm::vec4> shotpoints;

//Interseção esfera;
glm::vec4 esferacentro;
float raioesfera;

// Funcões de contato/intersecção
// Intersecção cubo-plano
bool isPlaneBox(glm::vec4 boxmin,glm::vec4 boxmax)
{
  return (boxmin.y <= -1.0f);
}
// Intersecção ponto-esfera
bool isPointCircle(glm::vec4 point,glm::vec4 circle,float raio)
{
  float distance =norm((point-circle));
  return distance<raio;
}
// Intersecção cubo-cubo
bool boxintersect(glm::vec4 boxmin1,glm::vec4 boxmax1,glm::vec4 boxmin2,glm::vec4 boxmax2)
{
    return (((((boxmin1.x>=boxmin2.x) && (boxmin1.x<=boxmax2.x)) ||
              ((boxmax1.x>=boxmin2.x) && (boxmax1.x<=boxmax2.x))) ||
             (((boxmin1.x<=boxmin2.x) && (boxmin1.x>=boxmax2.x)) ||
              ((boxmax1.x<=boxmin2.x) && (boxmax1.x>=boxmax2.x)))) &&

            ((((boxmin1.y>=boxmin2.y) && (boxmin1.y<=boxmax2.y)) ||
              ((boxmax1.y>=boxmin2.y) && (boxmax1.y<=boxmax2.y))) ||
             (((boxmin1.y<=boxmin2.y) && (boxmin1.y>=boxmax2.y)) ||
              ((boxmax1.y<=boxmin2.y) && (boxmax1.y>=boxmax2.y)))) &&

            ((((boxmin1.z>=boxmin2.z) && (boxmin1.z<=boxmax2.z)) ||
              ((boxmax1.z>=boxmin2.z) && (boxmax1.z<=boxmax2.z))) ||
             (((boxmin1.z<=boxmin2.z) && (boxmin1.z>=boxmax2.z)) ||
              ((boxmax1.z<=boxmin2.z) && (boxmax1.z>=boxmax2.z)))));
}

//RANGE DO TIRO
std::vector<double> shotrange;

// Vetor de posição das vacas nas curvas bezier
glm::vec4 posicao_vaca;

// Booleanos para saber se a nave acertou um tiro
int vaca1_acertada = 0, vaca2_acertada = 0, esfera_acertada = 0;

// Booleano para saber se a nave colidiu contra um objeto
int nave_bateu = 0;

// String de mensagens na tela da aplicação:
int texto = 0;

const std::string inicio = "Bem vindo a simulacao do aviao de batalha da forca aerea brasileira!";
const std::string inicio2 = "Voce foi selecionado para ser um dos nossos novos pilotos, entretanto,";
const std::string inicio3 = "como ainda nao possui experiencia com as nossas aeronaves, voce deve passar";
const std::string inicio4 = "primeiro por nossa simulacao. Para comecar, aperte a tecla 'L'.";

const std::string nave_parada = "Otimo. Esta eh a visao da nave em terceira pessoa. Voce podera controlar ela";
const std::string nave_parada2 = "livremente. Para fazer ela andar, segure 'W' e a aeronave comecara a acelerar.";
const std::string nave_parada3 = "Conforme mais voce segura a tecla, mais ela acelerara ate a velocidade limite.";
const std::string nave_parada4 = "Use as teclas 'A' e 'D' para mover a nave para a esquerda e direita, respectivamente,";
const std::string nave_parada5 = "e use a movimentação para cima e para baixo do mouse para descer e subir a nave";
const std::string nave_parada6 = "respectivamente. Por agora, tente chegar a velocidade maxima da aeronave.";

const std::string nave_em_movimento = "Excelente! Eh importante voce nao colidir a nave no chao ou em qualquer objeto que";
const std::string nave_em_movimento1 = "esteja voando. Isso significara o fim da simulacao e voce nao estara apto em pilotar";
const std::string nave_em_movimento2 = "uma aeronave de verdade. Voce pode parar a aeronave no ar quando quiser ao segurar a";
const std::string nave_em_movimento3 = "tecla 'S', ela ira diminuir a velocidade da nave ate que pare, tente fazer isso.";

const std::string tiro_esfera = "Excelente! Em seguida, vamos tentar atirar com a aeronave, use a tecla espaco para atirar.";
const std::string tiro_esfera2 = "Para seguirmos para a próxima parte da simulação, tenta achar uma esfera, mire e atire nela ate estourar.";
const std::string tiro_esfera3 = "Nao se preocupe, voce pode colidir com ela por enquanto.";

const std::string tiro_vacas = "Bom tiro, vamos exercitar um pouco mais. Dessa vez tentar alvos mais dificeis para acertar.";
const std::string tiro_vacas2 = "Encontre as 2 vacas que estao voando pelo ceu e tenta acerta-las, elas se movimentam bem rapido,";
const std::string tiro_vacas3 = "entao tome cuidado e tente nao ser acertado por elas tambem.";

const std::string fim = "Parabens, voce concluiu todos os testes da simulacao e esta pronto para pilotar";
const std::string fim2 = "uma aeronave de verdade, pressione esc para sair da simulacao ou 'U' para tentar novamente.";

const std::string colisao_detectada = "Voce colidiu a nave! Voce nunca ira ser um piloto se continuar assim.";
const std::string colisao_detectada2 = "Pressione 'U' para reiniciar a simulacao ou pressione esc para sair.";

// Booleano para indicar o fim do programa
int end_of_program = 0;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

// Tamanho da esfera
float sphere_size = 0.3;

int main(int argc, char* argv[])
{

    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Battle Ship", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 217-219 do documento "Aula_03_Rendering_Pipeline_Grafico.pdf".
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/porto-alegre.jpg");      // TextureImage0
    LoadTextureImage("../../data/metal_texture.jpg");  // TextureImage1

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel shipmodel("../../data/arwing SNES.obj","../../data/");
    ComputeNormals(&shipmodel);
    BuildTrianglesAndAddToVirtualScene(&shipmodel);

    ObjModel planemodel("../../data/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel cow("../../data/cow.obj");
    ComputeNormals(&cow);
    BuildTrianglesAndAddToVirtualScene(&cow);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slide 108 do documento "Aula_09_Projecoes.pdf".
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 22-34 do documento "Aula_13_Clipping_and_Culling.pdf".
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Variáveis auxiliares utilizadas para chamada à função
    // TextRendering_ShowModelViewProjection(), armazenando matrizes 4x4.
    glm::mat4 the_projection;
    glm::mat4 the_model;
    glm::mat4 the_view;

    int itiro = 0;      // Booleano para o tiro da nave
    float rotation = 0.0; //Rotação da nave baseada no precionamento de direita e esquerda
    glm::mat4 model_free_camera;

    // bbox dos objetos
    glm::vec4 cow1_bbox_min_const = glm::vec4(g_VirtualScene["cow"].bbox_min.x,g_VirtualScene["cow"].bbox_min.y,g_VirtualScene["cow"].bbox_min.z,1.0f);
    glm::vec4 cow1_bbox_min;
    glm::vec4 cow1_bbox_max_const = glm::vec4(g_VirtualScene["cow"].bbox_max.x,g_VirtualScene["cow"].bbox_max.y,g_VirtualScene["cow"].bbox_max.z,1.0f);
    glm::vec4 cow1_bbox_max;

    glm::vec4 cow2_bbox_min_const = cow1_bbox_min_const;
    glm::vec4 cow2_bbox_min;
    glm::vec4 cow2_bbox_max_const = cow1_bbox_max_const;
    glm::vec4 cow2_bbox_max;

    glm::vec4 nave_bbox_max_const = glm::vec4(g_VirtualScene["Arwing_SNES_Vert.001"].bbox_max.x,g_VirtualScene["Arwing_SNES_Vert.001"].bbox_max.y,g_VirtualScene["Arwing_SNES_Vert.001"].bbox_max.z,1.0f);
    glm::vec4 nave_bbox_min_const = glm::vec4(g_VirtualScene["Arwing_SNES_Vert.001"].bbox_min.x,g_VirtualScene["Arwing_SNES_Vert.001"].bbox_min.y,g_VirtualScene["Arwing_SNES_Vert.001"].bbox_min.z,1.0f);
    glm::vec4 nave_bbox_max;
    glm::vec4 nave_bbox_min;

    glm::vec4 vaca1_centro;
    glm::vec4 vaca2_centro;

    float vaca1_raio;
    float vaca2_raio;

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {

        double tnow = glfwGetTime();
        deltat = tnow - tprev;
        tprev = tnow;
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(0.7f, 0.7f, 0.7f, 1.0f);

        // Fazemos a chamada da função de movimentação da nave, onde é calculada sua velocidade.
        Anda();

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(program_id);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        glm::vec4 camera_view_vector;
        glm::vec4 camera_up_vector;
        if(Look_at)
        {
            float r = g_CameraDistance;
            float y = r*sin(g_CameraPhi);
            float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
            float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

            // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
            // Veja slides 172-182 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".
            camera_position_c  =  glm::vec4(x,y,z,1.0f); // Ponto "c", centro da câmera
            glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_view_vector = camera_lookat_l -  camera_position_c ; // Vetor "view", sentido para onde a câmera está virada
            camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

            // Deltat é
            deltat=0.0f;
        }
        else
        {
            float r = 2.5f;
            float y = r;
            float z = r;
            float x = r;
            // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
            // Veja slides 165-175 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".
            glm::vec4 camera_lookat_l    = glm::vec4(x,y,z,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada

             camera_up_vector= glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
             camera_view_vector = Matrix_Rotate(g_CameraTheta,vector_v(glm::vec4(1.0f,0.0f,0.0f,0.0f),glm::vec4(0.0f,1.0f,0.0f,0.0f)))*
            Matrix_Rotate(g_CameraPhi,vector_u(glm::vec4(1.0f,0.0f,0.0f,0.0f),glm::vec4(0.0f,1.0f,0.0f,0.0f)))*
            glm::vec4(1.0f,0.0f,0.0f,0.0f);
            glm::vec4 camera_view_vector2 = camera_view_vector;
        }
        glm::vec4 v = vector_v(camera_view_vector,camera_up_vector);
        w = vector_w(camera_view_vector,camera_up_vector);
        u = vector_u(camera_view_vector,camera_up_vector);

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slide 186 do documento "Aula_08_Sistemas_de_Coordenadas.pdf".
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 190-193 do documento "Aula_09_Projecoes.pdf".
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -50.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slide 227 do documento "Aula_09_Projecoes.pdf".
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slide 236 do documento "Aula_09_Projecoes.pdf".
            // Para s20*deltatimular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }
        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        #define SPHERE 0
        #define SHIP 1
        #define PLANE  2
        #define COW 3
        #define COWTWO 4
        /*
        // Desenhamos o modelo da esfera
        model = Matrix_Translate(-1.0f,0.0f,0.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, SPHERE);
        DrawVirtualObject("sphere");
        */
        int dx = anda_esquerda-anda_direita;
        if(dx == 0)
        {
            if(rotation == 0)
            {
                rotation = 0;
            }
            else
            {
                if(rotation > 0)
                {
                    rotation -= 100*(float)deltat;
                    if(rotation < 0)
                    {
                        rotation = 0;
                    }
                }
                else
                {
                    rotation += 100*(float)deltat;
                    if(rotation > 0)
                    {
                        rotation = 0;
                    }
                }
            }
        }
        if(dx > 0)
        {
            if(rotation < ROTATELIMIT)
            {
                rotation += 100*(float)deltat;
            }
        }
        else if(dx < 0)
        {
            if(rotation >- ROTATELIMIT)
            {
                rotation -= 100*(float)deltat;
            }
        }



        if(Look_at)
        {
            model = Matrix_Scale(0.04f,0.04f,0.04f);
            /*model = Matrix_Scale(0.04f,0.04f,0.04f)*Matrix_Rotate_Y(0.0f)*
                    Matrix_Rotate_Z(g_CameraPhi)*Matrix_Rotate_Y(3.14+3.14/2)*Matrix_Rotate_Z((rotation)*((3.14/2)*0.8)/ROTATELIMIT);*/

            // Define as medidas de bbox da nave a partir das modificações no modelo
            nave_bbox_max = Matrix_Scale(0.04f,0.04f,0.04f)*Matrix_Rotate_Y(0.0f)*
                                            Matrix_Rotate_Z(g_CameraPhi)*Matrix_Rotate_Y(3.14+3.14/2)*Matrix_Rotate_Z((rotation)*((3.14/2)*0.8)/ROTATELIMIT) * nave_bbox_max_const;

            nave_bbox_min = Matrix_Scale(0.04f,0.04f,0.04f)*Matrix_Rotate_Y(0.0f)*
                                            Matrix_Rotate_Z(g_CameraPhi)*Matrix_Rotate_Y(3.14+3.14/2)*Matrix_Rotate_Z((rotation)*((3.14/2)*0.8)/ROTATELIMIT) * nave_bbox_min_const;
        }
        else
        {
            glm::vec3 up_ArWing=Matrix_Rotate_Y(g_CameraTheta)*Matrix_Rotate_Z(g_CameraPhi)*Matrix_Rotate_Y(3.14+3.14/2)*glm::vec4(0.0f,-0.3f,0.0f,0.0f);
            model = Matrix_Translate(camera_position_c.x+camera_view_vector.x+up_ArWing.x,
                    camera_position_c.y+camera_view_vector.y+up_ArWing.y,
                    camera_position_c.z+camera_view_vector.z+up_ArWing.z)*
                    Matrix_Scale(0.04f,0.04f,0.04f)*Matrix_Rotate_Y(g_CameraTheta)*
                    Matrix_Rotate_Z(g_CameraPhi)*Matrix_Rotate_Y(3.14+3.14/2)*Matrix_Rotate_Z((rotation)*((3.14/2)*0.8)/ROTATELIMIT);
            model_free_camera = model;

            // Define as medidas de bbox da nave a partir das modificações no modelo
            nave_bbox_max = model * nave_bbox_max_const;

            nave_bbox_min = model * nave_bbox_min_const;
        }

        if(!nave_bateu)
        {
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(object_id_uniform, SHIP);
            DrawVirtualObject("Arwing_SNES_Vert.001");
        }
        else
        {
            texto = 6;
        }

        // Desenhamos o modelo do plano do chão
        model = Matrix_Translate(0.0f,-1.0f,0.0f)
              * Matrix_Scale(30.0f,1.0f,30.0f);
        glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(object_id_uniform, PLANE);
        DrawVirtualObject("plane");

        if(texto == 4 && !vaca1_acertada)
        {
            // Vaca 1
            posicao_vaca = curva_bezier(1);
            model = Matrix_Translate(posicao_vaca.x,posicao_vaca.y,posicao_vaca.z)*Matrix_Scale(1.0f,1.0f,1.0f)*Matrix_Rotate_Y(PI/2);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, COW);
            DrawVirtualObject("cow");
            glm::vec4 posicao_vaca;

            //termina modelo de boxman e boxmin da primeira vaca
            cow1_bbox_min = model * cow1_bbox_min_const;
            cow1_bbox_max = model * cow1_bbox_max_const;
            // Centro da vaca 1
            vaca1_centro = (cow1_bbox_max + cow1_bbox_min) * 0.5f;
            // Raio da vaca 1
            vaca1_raio = norm(vaca1_centro - cow1_bbox_min);
        }

        if(texto == 4 && !vaca2_acertada)
        {
            // Vaca 2
            posicao_vaca = curva_bezier(2);
            model = Matrix_Translate(posicao_vaca.x,posicao_vaca.y,posicao_vaca.z)*Matrix_Scale(1.0f,1.0f,1.0f)*Matrix_Rotate_Y(-PI/2);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, COWTWO);
            DrawVirtualObject("cow");

            //termina modelo de boxman e boxmin da segunda vaca
            cow2_bbox_min = model * cow2_bbox_min_const;
            cow2_bbox_max = model * cow2_bbox_max_const;
            // Centro vaca 2
            vaca2_centro = (cow2_bbox_max + cow2_bbox_min) * 0.5f;
            // Raio da vaca 2
            vaca2_raio = norm(vaca2_centro - cow2_bbox_min);
        }


        // Detecta se ouve tiro ou não
        if(atira && !Look_at)
        {
            primeiro=1;
            atira=0;
            itiro+=1;
            tiros.push_back(std::make_pair(model_free_camera * Matrix_Scale(0.3,0.3,0.3),glm::vec4(camera_view_vector.x,camera_view_vector.y,camera_view_vector.z,0.0)));
            shotpoints.push_back(glm::vec4(10000.0f,10000.0f,10000.0f,10000.0f));
            shotrange.push_back(0);
        }
        for(int i=0;i<tiros.size();i++)
        {
            // 25 = velocidade de tiro
            tiros[i].first = Matrix_Translate(tiros[i].second.x*25*deltat,tiros[i].second.y*25*deltat,tiros[i].second.z*25*deltat)*tiros[i].first;
            shotrange[i]+= 5*deltat;
            if(shotrange[i] > 10)
            {
                tiros.erase(std::remove(tiros.begin(), tiros.end(), tiros[i]), tiros.end());
                shotrange.erase(std::remove(shotrange.begin(), shotrange.end(), shotrange[i]), shotrange.end());
                last_i = -1;
            }
            model = tiros[i].first;
            shotpoints[i] = model*glm::vec4(0.0f,0.0f,0.0f,1.0f);
            glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(object_id_uniform, SPHERE);
            DrawVirtualObject("sphere");

            //TESTES DE INTESEÇÃO BALAS
            if(isPointCircle(shotpoints[i],vaca1_centro,vaca1_raio) && (texto == 4) && !vaca1_acertada)
            {
                 vaca1_acertada = 1;
            }
            if(isPointCircle(shotpoints[i],vaca2_centro,vaca2_raio) && (texto == 4) && !vaca2_acertada)
            {
                 vaca2_acertada = 1;
            }
            if(isPointCircle(shotpoints[i],esferacentro,raioesfera) && (texto == 3))
            {
                if(last_i != i)
                {
                    sphere_size = sphere_size + 0.1;
                    if(sphere_size >= 1.5)
                        texto = 4;
                    last_i = i;
                }
            }
        }

        //testa se tocou uma vaquinha
        if(((boxintersect(nave_bbox_min,nave_bbox_max,cow1_bbox_min,cow1_bbox_max) && !vaca1_acertada) ||
            boxintersect(nave_bbox_min,nave_bbox_max,cow2_bbox_min,cow2_bbox_max) && !vaca2_acertada) && !nave_bateu)
        {
            nave_bateu = 1;
        }

        //testa se tocou o plano
        if(isPlaneBox(nave_bbox_min,nave_bbox_max) && !nave_bateu)
        {
            nave_bateu = 1;
        }

        // Se o usuário acertar a esfera, ela cresce até ser estourada
        if(texto == 3)
        {
            model = Matrix_Translate(0.5f,1.0f,1.0f)*Matrix_Scale(sphere_size,sphere_size,sphere_size);
            esferacentro = model*glm::vec4(0.0f,0.0f,0.0f,1.0f);
            glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(object_id_uniform, SPHERE);
            DrawVirtualObject("sphere");
            raioesfera = sphere_size;
        }

        // Mensagens da tela
        switch(texto)
        {
        case 0:
            TextRendering_PrintString(window, inicio, -1.0, 0.95, 1.0);
            TextRendering_PrintString(window, inicio2, -1.0, 0.90, 1.0);
            TextRendering_PrintString(window, inicio3, -1.0, 0.85, 1.0);
            TextRendering_PrintString(window, inicio4, -1.0, 0.80, 1.0);
            break;
        case 1:
            TextRendering_PrintString(window, nave_parada, -1.0, 0.95, 1.0);
            TextRendering_PrintString(window, nave_parada2, -1.0, 0.90, 1.0);
            TextRendering_PrintString(window, nave_parada3, -1.0, 0.85, 1.0);
            TextRendering_PrintString(window, nave_parada4, -1.0, 0.80, 1.0);
            TextRendering_PrintString(window, nave_parada5, -1.0, 0.75, 1.0);
            TextRendering_PrintString(window, nave_parada6, -1.0, 0.70, 1.0);
            break;
        case 2:
            TextRendering_PrintString(window, nave_em_movimento, -1.0, 0.95, 1.0);
            TextRendering_PrintString(window, nave_em_movimento1, -1.0, 0.90, 1.0);
            TextRendering_PrintString(window, nave_em_movimento2, -1.0, 0.85, 1.0);
            TextRendering_PrintString(window, nave_em_movimento3, -1.0, 0.80, 1.0);
            break;
        case 3:
            TextRendering_PrintString(window, tiro_esfera, -1.0, 0.95, 1.0);
            TextRendering_PrintString(window, tiro_esfera2, -1.0, 0.90, 1.0);
            TextRendering_PrintString(window, tiro_esfera3, -1.0, 0.85, 1.0);
            break;
        case 4:
            TextRendering_PrintString(window, tiro_vacas, -1.0, 0.95, 1.0);
            TextRendering_PrintString(window, tiro_vacas2, -1.0, 0.90, 1.0);
            TextRendering_PrintString(window, tiro_vacas3, -1.0, 0.85, 1.0);
            break;
        case 5:
            TextRendering_PrintString(window, fim, -1.0, 0.95, 1.0);
            TextRendering_PrintString(window, fim2, -1.0, 0.90, 1.0);
            break;
        case 6:
            TextRendering_PrintString(window, colisao_detectada, -1.0, 0.95, 1.0);
            TextRendering_PrintString(window, colisao_detectada2, -1.0, 0.90, 1.0);
        }

        if(vaca1_acertada && vaca2_acertada)
            texto = 5;

        // Pegamos um vértice com coordenadas de modelo (0.5, 0.5, 0.5, 1) e o
        // passamos por todos os sistemas de coordenadas armazenados nas
        // matrizes the_model, the_view, e the_projection; e escrevemos na tela
        // as matrizes e pontos resultantes dessas transformações.
        //glm::vec4 p_model(0.5f, 0.5f, 0.5f, 1.0f);
        //TextRendering_ShowModelViewProjection(window, projection, view, model, p_model);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // o framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();

        if(end_of_program)
        {
            glfwTerminate();
            return 0;
        }
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 100 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura. Falaremos sobre eles em uma próxima aula.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 217-219 do documento "Aula_03_Rendering_Pipeline_Grafico.pdf".
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform  = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform  = glGetUniformLocation(program_id, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}
void ComputeNormalsPhong(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gourad, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            // Cálculo da normal de um triângulo cujos vértices
            // estão nos pontos "a", "b", e "c", definidos no sentido anti-horário.
            const glm::vec4  n = crossproduct(b - a, c - a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gourad, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            // PREENCHA AQUI o cálculo da normal de um triângulo cujos vértices
            // estão nos pontos "a", "b", e "c", definidos no sentido anti-horário.
            const glm::vec4  n = crossproduct(b - a, c - a);
            // preenchido

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = (void*)first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula (slides 33-44 do documento "Aula_07_Transformacoes_Geometricas_3D.pdf").
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slide 227 do documento "Aula_09_Projecoes.pdf".
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (!g_LeftMouseButtonPressed)
        return;

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dy = ypos - g_LastCursorPosY;
    float dx = xpos - g_LastCursorPosX;

    // Atualizamos parâmetros da câmera com os deslocamentos
    // Mudou de (-) para (+) para inverter os controles da camera que também controlam a nave,
    // para dar a sensação de um controle de manche de um avião/nave
    if(!Look_at)
        g_CameraPhi   += 1.5f*dy*deltat;
    else
    {
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;
    }

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
   if(key==GLFW_KEY_E && action ==GLFW_PRESS){
        rotateR=1;
    }
    if (key==GLFW_KEY_Q &&action ==GLFW_PRESS){
        rotateL=1;
    }
    if ((key == GLFW_KEY_W && action == GLFW_PRESS) ||
        ((key == GLFW_KEY_W && action == GLFW_PRESS) && (key == GLFW_KEY_A && action == GLFW_PRESS)) ||
        ((key == GLFW_KEY_W && action == GLFW_PRESS) && (key == GLFW_KEY_D && action == GLFW_PRESS)) ||
        ((key == GLFW_KEY_W && action == GLFW_PRESS) && (key == GLFW_KEY_A && action == GLFW_RELEASE)) ||
        ((key == GLFW_KEY_W && action == GLFW_PRESS) && (key == GLFW_KEY_D && action == GLFW_RELEASE)))
    {
        acelerando = 1;
    }
    if ((key == GLFW_KEY_S && action == GLFW_PRESS) ||
        ((key == GLFW_KEY_S && action == GLFW_PRESS) && (key == GLFW_KEY_A && action == GLFW_PRESS)) ||
        ((key == GLFW_KEY_S && action == GLFW_PRESS) && (key == GLFW_KEY_D && action == GLFW_PRESS)) ||
        ((key == GLFW_KEY_S && action == GLFW_PRESS) && (key == GLFW_KEY_A && action == GLFW_RELEASE)) ||
        ((key == GLFW_KEY_S && action == GLFW_PRESS) && (key == GLFW_KEY_D && action == GLFW_RELEASE)))
    {
        freando = 1;            // Começa a parar de acelerar
    }
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
         anda_esquerda=1;
    }
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
         anda_direita=1;
    }
    if (key == GLFW_KEY_E && action == GLFW_RELEASE)
    {
        rotateR=0;
    }
    if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
    {
        rotateL=0;
    }
     if ((key == GLFW_KEY_W && action == GLFW_RELEASE) ||
         ((key == GLFW_KEY_W && action == GLFW_RELEASE) && (key == GLFW_KEY_A && action == GLFW_PRESS)) ||
         ((key == GLFW_KEY_W && action == GLFW_RELEASE) && (key == GLFW_KEY_D && action == GLFW_PRESS)) ||
         ((key == GLFW_KEY_W && action == GLFW_RELEASE) && (key == GLFW_KEY_A && action == GLFW_RELEASE)) ||
         ((key == GLFW_KEY_W && action == GLFW_RELEASE) && (key == GLFW_KEY_D && action == GLFW_RELEASE)))
    {
        acelerando = 0;                // Agora a nave está parando de acelerar
    }
    if ((key == GLFW_KEY_S && action == GLFW_RELEASE) ||
        ((key == GLFW_KEY_S && action == GLFW_RELEASE) && (key == GLFW_KEY_A && action == GLFW_PRESS)) ||
        ((key == GLFW_KEY_S && action == GLFW_RELEASE) && (key == GLFW_KEY_D && action == GLFW_PRESS)) ||
        ((key == GLFW_KEY_S && action == GLFW_RELEASE) && (key == GLFW_KEY_A && action == GLFW_RELEASE)) ||
        ((key == GLFW_KEY_S && action == GLFW_RELEASE) && (key == GLFW_KEY_D && action == GLFW_RELEASE)))
    {
        freando = 0;                // Agora a nave está parando de frear
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        anda_esquerda=0;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        anda_direita=0;
    }
    if(primeiro==1){
        atira=0;
    }
    // Se o usuário apertar a tecla espaço, a nave do jogo atira
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS &&primeiro!= 1)
    {
        atira = 1;
        primeiro=1;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
    {
        primeiro=0;
        atira = 0;
    }
    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS&&Look_at)
    {
        Look_at = false;
        if(texto == 0)
            texto = 1;
        g_CameraTheta=0;
        g_CameraPhi=0;
        camera_position_c=glm::vec4(0.0f,0.0f,0.0f,1.0f);
    }
    // Se o usuário apertar a tecla ESC, o programa encerrará
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwDestroyWindow(window);
        end_of_program = 1;
    }
        // Se o usuário apertar a tecla U, a simulacao será reiniciada.
    if (key == GLFW_KEY_U && action == GLFW_PRESS)
    {
        Look_at = true;
        nave_bateu = 0;
        vaca1_acertada = 0;
        vaca2_acertada = 0;
        esfera_acertada = 0;
        texto = 0;
        g_CameraTheta=0;
        g_CameraPhi=0;
        camera_position_c=glm::vec4(0.0f,0.0f,0.0f,1.0f);
        acelera_frente = 0.0f;
        sphere_size = 0.3;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     World", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     Camera", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                   NDC", -1.0f, 1.0f-13*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-14*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

void Anda()
{
    float dx = anda_direita - anda_esquerda;
    g_CameraTheta -= 1.5f*dx*deltat;
    if(acelerando)                                  // Enquanto W estiver sendo pressionado
    {
        if(acelera_frente <= 0.05 && !Look_at)     // E se a nave não chegou na velocidade máxima
            acelera_frente += 0.01*deltat;        //aumenta a velocidade dela.
        else
            if(texto == 1)
                texto = 2;
        anda_cima = 1;                              // Permite a nave andar para frente
    }
    if(freando)                                     // Enquanto W estiver sendo pressionado
    {
        if (acelera_frente >= 0.0000)               // Se a nave não chegou na velocidade mínima
            acelera_frente -= 0.02*deltat;        // diminui a velocidade
        else                                        // Se a nave está na velocidade mínima (parada)
        {
            anda_cima = 0;                          // ela para de se mover
            if(texto == 2)
                texto = 3;
        }
    }
    if (anda_cima == 1)
    {
        if(!nave_bateu)
            camera_position_c -= acelera_frente * w;    // Faz o deslocamento da câmera (nave).
    }
    if(rotateR == 1){
        rotationX+=0.1f*deltat;
    }
    if(rotateL == 1 ){
        rotationX-=0.1f*deltat;
    }
}

// Curva Bezier circular
glm::vec4 curva_bezier(int which_cow)
{
    // Ponto a ser retornado
    glm::vec4 Ct;

    // Velocidade de deslocamento
    float deslocamento = 0.2;

    // Apotema do triangulo que definirá nossa curva Bezier Circular
    float apothem_x = 0.0f;
    float apothem_y = 12.0f;
    float apothem_z;

    // Altura do triangulo
    float height = 30.0f;

    // Vaca 1
    if(which_cow == 1)
        apothem_z = 15.0f;

    // Vaca 2
    else
        apothem_z = -15.0f;

    float lado = height/(sqrt(3)/2);

    glm::vec4 apothem = glm::vec4(apothem_x, apothem_y, apothem_z, 1.0f);

    // Construção do triangulo a partir do apotema
    glm::vec4 ponto1    = glm::vec4(apothem_x, apothem_y - (height/3), apothem_z, 1.0f);                    // Ponto no meio da base do triangulo
    glm::vec4 ponto2    = glm::vec4(apothem_x + (lado/2), apothem_y - (height/3), apothem_z, 1.0f);         // Vértice 1 do triangulo
    glm::vec4 ponto3    = glm::vec4(apothem_x, apothem_y + (2 * height/3), apothem_z, 1.0f);                // Vértice 2 do triangulo
    glm::vec4 ponto4    = glm::vec4(apothem_x - (lado/2), apothem_y - (height/3), apothem_z, 1.0f);         // Vértice 3 do triangulo

    // Parametrização da curva:
    if(which_cow == 1)
    {
        glm::vec4 C12 = ponto1 + valor_param_vaca1*(ponto2 - ponto1);
        glm::vec4 C23 = ponto2 + valor_param_vaca1*(ponto3 - ponto2);
        glm::vec4 C34 = ponto3 + valor_param_vaca1*(ponto4 - ponto3);
        glm::vec4 C41 = ponto4 + valor_param_vaca1*(ponto1 - ponto4);
        glm::vec4 C123 = C12 + valor_param_vaca1*(C23 - C12);
        glm::vec4 C234 = C23 + valor_param_vaca1*(C34 - C23);
        glm::vec4 C341 = C34 + valor_param_vaca1*(C41 - C34);
        glm::vec4 C1234 = C123 + valor_param_vaca1*(C234 - C123);
        glm::vec4 C2341 = C234 + valor_param_vaca1*(C341 - C234);
        Ct = C1234 + valor_param_vaca1*(C2341 - C1234);

        // Atualizamos o parametro da curva
        if(valor_param_vaca1 < 1.0f)
            valor_param_vaca1 = valor_param_vaca1 + deslocamento * deltat;
        else
            valor_param_vaca1 = 0.0f;
    }
    else
    {
        glm::vec4 C12 = ponto1 + valor_param_vaca2*(ponto2 - ponto1);
        glm::vec4 C23 = ponto2 + valor_param_vaca2*(ponto3 - ponto2);
        glm::vec4 C34 = ponto3 + valor_param_vaca2*(ponto4 - ponto3);
        glm::vec4 C41 = ponto4 + valor_param_vaca2*(ponto1 - ponto4);
        glm::vec4 C123 = C12 + valor_param_vaca2*(C23 - C12);
        glm::vec4 C234 = C23 + valor_param_vaca2*(C34 - C23);
        glm::vec4 C341 = C34 + valor_param_vaca2*(C41 - C34);
        glm::vec4 C1234 = C123 + valor_param_vaca2*(C234 - C123);
        glm::vec4 C2341 = C234 + valor_param_vaca2*(C341 - C234);
        Ct = C1234 + valor_param_vaca2*(C2341 - C1234);

        // Atualizamos o parametro da curva
        if(valor_param_vaca2 > 0.0f)
            valor_param_vaca2 = valor_param_vaca2 - deslocamento * deltat;
        else
            valor_param_vaca2 = 1.0f;
    }

    // Retornamos o ponto atual
    return Ct;
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
