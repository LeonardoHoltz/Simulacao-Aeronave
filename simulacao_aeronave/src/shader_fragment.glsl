#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;
in vec3 colorGourad;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define SHIP 1
#define PLANE  2
#define COW 3
#define COWTWO 4
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,0.5,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    // Angulos para projeção esférica de textura
    float ro;
    float theta;
    float phi;

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = normalize(-l + 2*n*(dot(n,l)));

    // Parâmetros que definem as propriedades espectrais da superfície
    vec3 Kd; // Refletância difusa
    vec3 Ks; // Refletância especular
    vec3 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    if ( object_id == SPHERE )
    {
        // Propriedades espectrais da esfera
        Kd = vec3(1.0,0.0,0.0);
        Ks = vec3(1.0,0.0,0.0);
        Ka = vec3(1.0,0.0,0.0);
        q = 10.0;
    }
    else if ( object_id == SHIP )
    {
        // Propriedades espectrais da nave
        Kd = vec3(0.5,0.5,0.5);
        Ks = vec3(0.8,0.8,0.8);
        Ka = vec3(0.5,0.5,0.5);
        q = 32.0;
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;

        vec4 p2 = position_model - bbox_center;
        ro = length(p2);
        theta = atan(p2.x, p2.z);
        phi = asin(p2.y/ro);

        U = (theta + M_PI)/(2 * M_PI);
        V = (phi + M_PI_2)/M_PI;
    }
    else if ( object_id == PLANE )
    {
        // Propriedades espectrais do plano
        Kd = vec3(0.2,0.2,0.2);
        Ks = vec3(0.3,0.3,0.3);
        Ka = vec3(0.0,0.0,0.0);
        q = 20.0;
        U = texcoords.x;
        V = texcoords.y;
    }
    else if(object_id == COWTWO)
    {
        Kd = vec3(0.2,0.9,0.02);
        Ks = vec3(1.0,1.0,1.0);
        Ka = vec3(0.2,0.9,0.02);
        q = 10.0;
    }
    else // Objeto desconhecido = preto
    {
        Kd = vec3(0.0,0.0,0.0);
        Ks = vec3(0.0,0.0,0.0);
        Ka = vec3(0.0,0.0,0.0);
        q = 1.0;
    }

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0); // PREENCHA AQUI o espectro da fonte de luz
    // preenchido

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.2,0.2,0.2); // PREENCHA AQUI o espectro da luz ambiente
    // preenchido

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd*I*max(0,dot(n,l)); // PREENCHA AQUI o termo difuso de Lambert
    // preenchido

    // Termo ambiente
    vec3 ambient_term = Ka*Ia; // PREENCHA AQUI o termo ambiente
    // preenchido

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks*I*pow(max(0, dot(r,v)),q); // PREENCHA AQUI o termo especular de Phong
    // preenchido

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
    vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage1
    vec3 Kd1 = texture(TextureImage1, vec2(U,V)).rgb;

    // Obtemos a refletância difusa a partir da leitura da imagem TextureImage2
    vec3 Kd2 = texture(TextureImage2, vec2(U,V)).rgb;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    if(object_id == COWTWO)
    {
        // Cor de fragmento final, juntando as 3 equaçoes
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
        // Cor final com correção gamma, considerando monitor sRGB
        color = pow(color, vec3(1.0,1.0,1.0)/2.2);
    }
    if(object_id == SHIP)
    {
        // Cor de fragmento final, juntando as 3 equaçoes
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
        color = Kd2 * color;
        // Cor final com correção gamma, considerando monitor sRGB
        color = pow(color, vec3(1.0,1.0,1.0)/2.2);
    }
    if(object_id == SPHERE)
    {
        // Cor de fragmento final, juntando as 3 equaçoes
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
        // Cor final com correção gamma, considerando monitor sRGB
        color = pow(color, vec3(1.0,1.0,1.0)/2.2);
    }
    if (object_id == PLANE)
    {
        // Cor de fragmento final, juntando as 3 equaçoes
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
        color = Kd0 * colorGourad;
        // Cor final com correção gamma, considerando monitor sRGB
        color = pow(color,vec3(1.0,1.0,1.0)/2.2);
    }
    if(object_id == COW)
    {
        color = colorGourad;
    }
}

