/*!\file window.c
 *
 * \brief Walking on finite plane with skydome textured with a
 * triangle-edge midpoint-displacement algorithm.
 *
 * \author Farès BELHADJ, amsi@ai.univ-paris8.fr
 * \date February 9 2017
 */
#include <assert.h>
#include <math.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL2/SDL_image.h>
#include <time.h>
#include <SDL2/SDL_mixer.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <SDL2/SDL_image.h>

static void quit(void);
static void initGL(void);
static void initData(void);
static void resize(int w, int h);
static void idle(void);
static void keydown(int keycode);
static void keyup(int keycode);
static void draw(void);

/*!\brief opened window width */
static int _windowWidth = 800;
/*!\brief opened window height */
static int _windowHeight = 600;
/*!\brief Quad geometry Id  */
static GLuint _plane = 0;
/*!\brief Sphere geometry Id  */
static GLuint _sphere = 0;
/*!\brief GLSL program Id */
static GLuint _pId = 0;
/*!\brief plane texture Id */
static GLuint _planeTexId = 0;
/*!\brief sky texture Id */
static GLuint _skyTexId = 0;
/*!\brief plane scale factor */
static GLfloat _planeScale = 100.0;
/*!\brief boolean to toggle anisotropic filtering */
static GLboolean _anisotropic = GL_FALSE;
/*!\brief boolean to toggle mipmapping */
static GLboolean _mipmap = GL_FALSE;
/*!\brief boolean to toggle scene fog */
static GLboolean _fog = GL_FALSE;

//nombres de plantes
static int nb_plantes = 30;


static GLfloat _lumPos0[4] = {5000.0, 20.0, 30.0, 1.0};
static GLuint _movingTexId[6] = {0,1,2,3,4,5};
static GLuint _fixedTexId[4] = {0,1,2,3};


extern void assimpInit(const char * filename);
extern void assimpDrawScene(void);
extern void assimpQuit(void);

int nbanimal = 2000;
int nb = 1000;

//////////////: STRUCTURE POUR CREER DES ENSEMBLES DE POINTS ////////////////////////

struct Animal
{
  int type;
  GLfloat x, y, z;
  GLfloat xx, yy, zz;
  GLboolean move;
};
typedef struct Animal Animal;
Animal* animal = NULL;



struct Fixedd
{
  int type;
  GLfloat x,z;
};
typedef struct Fixedd Fixedd;
Fixedd fixedd = NULL;


struct Plante{ 
  GLfloat x,z;
};
typedef struct Plante plante;
plante herbes[50];

//Les fonctions randoms
double random_range (double min, double max)
{
   return min + ((max - min) * (rand () / (double) RAND_MAX));
}

float myRand() {
	return rand() / (RAND_MAX + 1.0);
}
 
 
void place_herbes (){
  int i;
  for ( i = 0; i < nb_plantes; ++i){
    herbes[i].x = random_range(-20,50);
    herbes[i].z = random_range(-20,50);
  }
}



/*!\brief enum that index keyboard mapping for direction commands */
enum kyes_t {
  KLEFT = 0,
  KRIGHT,
  KUP,
  KDOWN
};

/*!\brief virtual keyboard for direction commands */
static GLuint _keys[] = {0, 0, 0, 0};

typedef struct cam_t cam_t;
/*!\brief a data structure for storing camera position and
 * orientation */
struct cam_t {
  GLfloat x, z;
  GLfloat theta;
};

/*!\brief the used camera */
static cam_t _cam = {0, 0, 0};




/*!\brief initializes OpenGL parameters :
 *
 * the clear color, enables face culling, enables blending and sets
 * its blending function, enables the depth test and 2D textures,
 * creates the program shader, the model-view matrix and the
 * projection matrix and finally calls resize that sets the projection
 * matrix and the viewport.
 */
static void initGL(void) {
  glClearColor(0.0f, 0.4f, 0.9f, 0.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  _pId  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  resize(_windowWidth, _windowHeight);
}






/*!\brief initializes data : 
 *
 * creates 3D objects (plane and sphere) and 2D textures.
 */
static void initData(void) {
  /* the checkboard texture */
  GLuint check[] = {-1, 255 << 24, 128 << 24, -1};

  /* a fractal texture generated usind a midpoint displacement algorithm */
  GLfloat * hm = gl4dmTriangleEdge(257, 257, 0.4);
  /* generates a quad using GL4Dummies */
  _plane = gl4dgGenQuadf();
  /* generates a sphere using GL4Dummies */
  _sphere = gl4dgGenQuadf();

  /* creation and parametrization of the plane texture */

//Le sol

SDL_Surface *sol = NULL;
sol= IMG_Load("img/sable.jpg");
  glGenTextures(1, &_planeTexId);
  glBindTexture(GL_TEXTURE_2D, _planeTexId);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
 if(sol) {
    printf("Sol : ok \n");
  #ifdef __APPLE__
     int mode = sol->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = sol->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sol->w, sol->h, 0, mode, GL_UNSIGNED_BYTE, sol->pixels);
      SDL_FreeSurface(sol);
  } 
  else{
    printf("Erreur\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }


 glGenTextures(1, &_skyTexId);
  glBindTexture(GL_TEXTURE_2D, _skyTexId);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 257, 257, 0, GL_RED, GL_FLOAT, hm);
  free(hm);
  



////////////// ELEMENTS IMMOBILES ////////////////////////



 SDL_Surface *cor = NULL;
cor = IMG_Load("img/algue.png");
 glGenTextures(1, &_fixedTexId[0]);
  glBindTexture(GL_TEXTURE_2D, _fixedTexId[0]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(cor) {
    printf("Algue :ok\n");
  #ifdef __APPLE__
     int mode = cor->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = cor->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, cor->w, cor->h, 0, mode, GL_UNSIGNED_BYTE, cor->pixels);
      SDL_FreeSurface(cor);
  } 
  else{
    printf("Erreur \n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }



 SDL_Surface *crabe= NULL;
 crabe = IMG_Load("img/crabe.png");
 glGenTextures(1, &_fixedTexId[1]);
  glBindTexture(GL_TEXTURE_2D, _fixedTexId[1]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(crabe) {
    printf("crabe: ok\n");
  #ifdef __APPLE__
     int mode = crabe->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = crabe->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, crabe->w, crabe->h, 0, mode, GL_UNSIGNED_BYTE, crabe->pixels);
      SDL_FreeSurface(crabe);
  } 
  else{
    printf("Erreur \n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }



SDL_Surface *rock = NULL;
rock = IMG_Load("img/roche.png");
 glGenTextures(1, &_fixedTexId[2]);
  glBindTexture(GL_TEXTURE_2D, _fixedTexId[2]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(rock) {
    printf("Roche : ok\n");
  #ifdef __APPLE__
     int mode = rock->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = rock->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rock->w, rock->h, 0, mode, GL_UNSIGNED_BYTE, rock->pixels);
      SDL_FreeSurface(rock);
  } 
  else{
    printf("Erreur \n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }



SDL_Surface *plante1 = NULL;
plante1= IMG_Load("img/plante.png");
 glGenTextures(1, &_fixedTexId[3]);
  glBindTexture(GL_TEXTURE_2D, _fixedTexId[3]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(plante1) {
    printf("Plante: ok\n");
  #ifdef __APPLE__
     int mode = plante1->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = plante1->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, plante1->w, crabe->h, 0, mode, GL_UNSIGNED_BYTE, plante1->pixels);
      SDL_FreeSurface(plante1);
  } 
  else{
    printf("Erreur \n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }

//////////////ELEMENTS EN MOUVEMENT ////////////////////////


  SDL_Surface *goldfish = NULL;
  goldfish = IMG_Load("img/goldfish.png");
  glGenTextures(1, &_movingTexId[0]);
  glBindTexture(GL_TEXTURE_2D, _movingTexId[0]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(goldfish) {
    printf("Goldfish: ok\\n");
  #ifdef __APPLE__
     int mode = goldfish->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = goldfish->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, goldfish->w, goldfish->h, 0, mode, GL_UNSIGNED_BYTE, goldfish->pixels);
      SDL_FreeSurface(goldfish);
  } 
  else{
    printf("Erreur\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }




  SDL_Surface *tortue = NULL;
  tortue = IMG_Load("img/tortue.png");
  glGenTextures(1, &_movingTexId[1]);
  glBindTexture(GL_TEXTURE_2D, _movingTexId[1]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(tortue) {
    printf("Tortue : ok\n");
  #ifdef __APPLE__
     int mode = tortue->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = tortue->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tortue->w, tortue->h, 0, mode, GL_UNSIGNED_BYTE, tortue->pixels);
      SDL_FreeSurface(tortue);
  } 
  else{
    printf("Erreur\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }




  SDL_Surface *hippocampe = NULL;
  hippocampe = IMG_Load("img/hippocampe.png");
  glGenTextures(1, &_movingTexId[2]);
  glBindTexture(GL_TEXTURE_2D, _movingTexId[2]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(hippocampe) {
    printf("Hippocampe : ok\n");
  #ifdef __APPLE__
     int mode = hippocampe->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = hippocampe->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, hippocampe->w, hippocampe->h, 0, mode, GL_UNSIGNED_BYTE, hippocampe->pixels);
      SDL_FreeSurface(hippocampe);
  } 
  else{
    printf("Erreur\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }


  SDL_Surface *meduse = NULL;
  meduse = IMG_Load("img/meduse.png");
  glGenTextures(1, &_movingTexId[3]);
  glBindTexture(GL_TEXTURE_2D, _movingTexId[3]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(meduse) {
    printf("Meduse : ok\n");
  #ifdef __APPLE__
     int mode = meduse->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = meduse->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, meduse->w, meduse->h, 0, mode, GL_UNSIGNED_BYTE, meduse->pixels);
      SDL_FreeSurface(meduse);
  } 
  else{
    printf("Erreur\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }

  
  SDL_Surface *dauphin = NULL;
  dauphin = IMG_Load("img/dauphin.png");
  glGenTextures(1, &_movingTexId[4]);
  glBindTexture(GL_TEXTURE_2D, _movingTexId[4]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(dauphin) {
    printf("Dauphin: ok\n");
  #ifdef __APPLE__
     int mode = dauphin->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = dauphin->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dauphin->w, dauphin->h, 0, mode, GL_UNSIGNED_BYTE, dauphin->pixels);
      SDL_FreeSurface(dauphin);
  } 
  else{
    printf("Erreur\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }

  SDL_Surface *bulle = NULL;
  bulle = IMG_Load("img/bulle.png");
  glGenTextures(1, &_movingTexId[5]);
  glBindTexture(GL_TEXTURE_2D, _movingTexId[5]);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  if(bulle) {
    printf("Bulle : ok\n");
  #ifdef __APPLE__
     int mode = bulle->format->BytesPerPixel == 4 ? GL_BGRA : GL_BGR;
  #else
      int mode = bulle->format->BytesPerPixel == 4 ? GL_RGBA : GL_RGB;
  #endif
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bulle->w, bulle->h, 0, mode, GL_UNSIGNED_BYTE, bulle->pixels);
      SDL_FreeSurface(bulle);
  } 
  else{
    printf("Erreur\n");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, check);
  }




}




/*!\brief function called by GL4Dummies' loop at resize. Sets the
 *  projection matrix and the viewport according to the given width
 *  and height.
 * \param w window width
 * \param h window height
 */
static void resize(int w, int h) {
  _windowWidth = w; 
  _windowHeight = h;
  glViewport(0, 0, _windowWidth, _windowHeight);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _windowHeight / _windowWidth, 0.5 * _windowHeight / _windowWidth, 1.0, _planeScale + 1.0);
  gl4duBindMatrix("modelViewMatrix");
}



/*!\brief function called by GL4Dummies' loop at key-down (key
 * pressed) event.
 * 
 * stores the virtual keyboard states (1 = pressed) and toggles the
 * boolean parameters of the application.
 */
static void keydown(int keycode) {
  GLint v[2];
  switch(keycode) {
  case SDLK_LEFT:
    _keys[KLEFT] = 1;
    break;
  case SDLK_RIGHT:
    _keys[KRIGHT] = 1;
    break;
  case SDLK_UP:
    _keys[KUP] = 1;
    break;
  case SDLK_DOWN:
    _keys[KDOWN] = 1;
    break;
  case SDLK_ESCAPE:
  case 'q':
    exit(0);
    /* when 'w' pressed, toggle between line and filled mode */
  case 'w':
    glGetIntegerv(GL_POLYGON_MODE, v);
    if(v[0] == GL_FILL) {
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glLineWidth(3.0);
    } else {
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      glLineWidth(1.0);
    }
    break;
  case 'f':
    _fog = !_fog;
    break;
    /* when 'm' pressed, toggle between mipmapping or nearest for the plane texture */
  case 'm': {
    _mipmap = !_mipmap;
    glBindTexture(GL_TEXTURE_2D, _planeTexId);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    break;
  }
    /* when 'a' pressed, toggle on/off the anisotropic mode */
  case 'a': {
    _anisotropic = !_anisotropic;
    /* l'Anisotropic sous GL ne fonctionne que si la version de la
       bibliothèque le supporte ; supprimer le bloc ci-après si
       problème à la compilation. */
#ifdef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
    GLfloat max;
    glBindTexture(GL_TEXTURE_2D, _planeTexId);
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, _anisotropic ? max : 1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif
    break;
  }
  default:
    break;
  }
}

/*!\brief function called by GL4Dummies' loop at key-up (key
 * released) event.
 * 
 * stores the virtual keyboard states (0 = released).
 */
static void keyup(int keycode) {
  switch(keycode) {
  case SDLK_LEFT:
    _keys[KLEFT] = 0;
    break;
  case SDLK_RIGHT:
    _keys[KRIGHT] = 0;
    break;
  case SDLK_UP:
    _keys[KUP] = 0;
    break;
  case SDLK_DOWN:
    _keys[KDOWN] = 0;
    break;
  default:
    break;
  }
}



////////////////////////TEXTURE////////////////////////


void movingObjet(GLfloat x, GLfloat z, int nb) {


  float r = rand()%21 + 1;

  gl4duPushMatrix(); {
    gl4duTranslatef(x, 1.0, z);
      gl4duRotatef(_cam.theta*180.0/3.14+90, 0, 1, 0);
    gl4duSendMatrices();
  } gl4duPopMatrix();

  gl4duPushMatrix(); {
    gl4duTranslatef(-x, r, z);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glCullFace(GL_BACK);
  

  glBindTexture(GL_TEXTURE_2D, _movingTexId[nb]);
  glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1.0);
  glUniform1i(glGetUniformLocation(_pId, "sky"), 0);
  gl4dgDraw(_sphere);


  
}


void fixedObjet(GLfloat x, GLfloat z, int nb) {

  gl4duPushMatrix(); {
    gl4duTranslatef(x, 1.0, z);
      gl4duRotatef(_cam.theta*180.0/3.14+90, 0, 1, 0);
    gl4duSendMatrices();
  } gl4duPopMatrix();

  gl4duPushMatrix(); {
    gl4duTranslatef(x, 1.0, z);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glCullFace(GL_BACK);
  

  glBindTexture(GL_TEXTURE_2D, _fixedTexId[nb]);
  glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1.0);
  glUniform1i(glGetUniformLocation(_pId, "sky"), 0);
  gl4dgDraw(_sphere);

  
}

//pour povoir inserer mon objet plante
void unObjetplante(GLfloat x, GLfloat z, int nb) {

  gl4duPushMatrix(); {
    gl4duTranslatef(x, 1.0, z);
      gl4duRotatef(_cam.theta*180.0/3.14+90, 0, 1, 0);
    gl4duSendMatrices();
    assimpDrawScene();
  } gl4duPopMatrix();

  gl4duPushMatrix(); {
    gl4duTranslatef(x, 1.0, z);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glCullFace(GL_BACK);
  

  glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1.0);
  glUniform1i(glGetUniformLocation(_pId, "sky"), 0);
  //gl4dgDraw(_sphere);

}


//////////////MOUVEMENT ////////////////////////



//vers la gauche
void movedauphin(double dt){

int i;
for ( i = 0; i < nbanimal; ++i)
{
	if (animal[i].type == 0)
  {
  	animal[i].x = animal[i].x - animal[i].xx * dt;
  	animal[i].y = animal[i].y - animal[i].yy * dt;

   }
}
 
}



//vers la droite 
void movegoldfish(double dt){

int i;
for (i = 0; i < nbanimal; ++i)
{
	if (animal[i].type == 1)
  {
  	animal[i].x = animal[i].x + animal[i].xx * dt;
  	animal[i].y = animal[i].y + animal[i].yy * dt;

   }
}
 
}

//vers la gauche
void movetortue(double dt){
int i;
for ( i = 0; i < nbanimal; ++i)
{
	if (animal[i].type == 2)
  {
  	animal[i].x = animal[i].x + animal[i].xx * dt;
  	animal[i].y = animal[i].y + animal[i].yy * dt;

   }
}
 
}

//vers la droite
void movebulle(double dt){
int i;

for ( i = 0; i < nbanimal; ++i){
	if (animal[i].type == 3)
  {
  	animal[i].x = animal[i].x + animal[i].xx * dt;
  	animal[i].y = animal[i].y + animal[i].yy * dt;
    	animal[i].z = animal[i].z + animal[i].zz * dt;

   }
}
 

}

//vers la gauche
void movehippocampe(double dt){
int i;
for (i = 0; i < nbanimal; ++i)
{
  if (animal[i].type == 4)
  {
    animal[i].x = animal[i].x + animal[i].xx * dt;
    animal[i].y = animal[i].y + animal[i].yy * dt;

   }
}
 
}


//vers la droite
void movemeduse(double dt){
int i;
for (i = 0; i < nbanimal; ++i)
{
  if (animal[i].type == 5)
  {
    animal[i].x = animal[i].x - animal[i].xx * dt;
    animal[i].y = animal[i].y - animal[i].yy * dt;

   }
}
 
}

//////////////ALLOCATION DYNAMIQUE DES STRUCTURES ////////////////////////


void fixedElements(){
fixedd=malloc(100000*sizeof(Fixedd));
srand(100000);
int i;
  for (i = 0; i < nb; ++i)
  {
    fixedd[i].x = 1000.0 * (2.0 * myRand() - 1.0);
    fixedd[i].z = 1000.0 * (2.0 * myRand() - 1.0); 
    fixedd[i].type = rand()%4;
 
  }
}

void movingElements(){
animal=malloc(300000*sizeof(animal));
srand(300000);
int i;
  for (i = 0; i < nbanimal; ++i)
  {

    animal[i].xx = 10.0;
    animal[i].yy = 0.0;
    animal[i].zz = 0.0;
    animal[i].x = 1000.0 * (2.0 * myRand() - 1.0);
    animal[i].z = 1000.0 * (2.0 * myRand() - 1.0); 
    animal[i].type = rand()%6;
    animal[i].y= 1000.0 * (2.0 * myRand() - 1.0);
    animal[i].move=GL_TRUE;
 
  }
}

//////////////COLLISION ////////////////////////


void fixedCollision(){

int i;
  for (i = 0; i < nbanimal; i++)
  {
    
    if (( (_cam.x <= fixedd[i].x + 10) && (_cam.x >= fixedd[i].x - 10)) 
      && ((_cam.z >= fixedd[i].z - 10) && (_cam.z <= fixedd[i].z + 10) && (fixedd[i].type == 1)))     {   

        //
    }
    

    if (( (_cam.x <= fixedd[i].x + 10) && (_cam.x >= fixedd[i].x - 10)) 
      && ((_cam.z >= fixedd[i].z - 10) && (_cam.z <= fixedd[i].z + 10) && (fixedd[i].type == 0)))     {  

        //
      }
     


    if (( (_cam.x <= fixedd[i].x + 10) && (_cam.x >= fixedd[i].x - 10)) 
      && ((_cam.z >= fixedd[i].z - 10) && (_cam.z <= fixedd[i].z + 10) && (fixedd[i].type == 2)))     {  

        //
    }

  }

}

void movingCollision(){
int i;

  for (i = 0; i < nbanimal; i++)
  {
    
    if (( (_cam.x <= animal[i].x + 1) && (_cam.x >= animal[i].x - 1)) 
      && ((_cam.z >= animal[i].z - 1) && (_cam.z <= animal[i].z + 1) && (animal[i].type == 1)))     {   
        animal[i].x = 1000.0 * (2.0 * myRand() - 1.0);
      	animal[i].z = 1000.0 * (2.0 * myRand() - 1.0); 
        

    }
    

    if (( (_cam.x <= animal[i].x + 1) && (_cam.x >= animal[i].x - 1)) 
      && ((_cam.z >= animal[i].z - 1) && (_cam.z <= animal[i].z + 1) && (animal[i].type == 0)))     {   
      animal[i].x = 1000.0 * (2.0 * myRand() - 1.0);
      animal[i].z = 1000.0 * (2.0 * myRand() - 1.0); 
         
    }
     


    if (( (_cam.x <= animal[i].x + 1) && (_cam.x >= animal[i].x - 1)) 
      && ((_cam.z >= animal[i].z - 1) && (_cam.z <= animal[i].z + 1) && (animal[i].type == 2)))     {   
       animal[i].x = 1000.0 * (2.0 * myRand() - 1.0);
       animal[i].z = 1000.0 * (2.0 * myRand() - 1.0); 


    }

  }

}



//////////////////////////////////////////////

//float vie = 1; variable que je voulais utiliser , pour que quand la cam touche 
//nemo , le programme s'arrête 

/*!\brief function called by GL4Dummies' loop at draw.*/
static void draw(void) {
  int xm, ym;
    static Uint32 t0 = 0, t;
  GLfloat dt = 0.0, lumPos[4], *mat;
  dt = ((t = SDL_GetTicks()) - t0) / 800.0;
  t0 = t;


  GLfloat cam[2] = {0, 0};
  /* pump the SDL input events (here for mouse) */
  SDL_PumpEvents();

  /* gets the mouse coordinates on the window */
  SDL_GetMouseState(&xm, &ym);
  /* clears the OpenGL color buffer and depth buffer */
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  /* sets the current program shader to _pId */
  glUseProgram(_pId);
  /* loads the identity matrix in the current GL4Dummies matrix ("modelViewMatrix") */
  gl4duLoadIdentityf();
  /* modifies the current matrix to simulate camera position and orientation in the scene */
  /* see gl4duLookAtf documentation or gluLookAt documentation */
  gl4duLookAtf(_cam.x, 1.0, _cam.z, 
         _cam.x - sin(_cam.theta), 1.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam.z - cos(_cam.theta), 
         0.0, 1.0,0.0);
  /* sets the current texture stage to 0 */
  glActiveTexture(GL_TEXTURE0);
  /* tells the pId program that "myTexture" is set to stage 0 */
  glUniform1i(glGetUniformLocation(_pId, "myTexture"), 0);
  /* tells the pId program what is the value of fog */
  glUniform1i(glGetUniformLocation(_pId, "fog"), _fog);

  /* pushs (saves) the current matrix (modelViewMatrix), scales,
   * rotates, sends matrices to pId and then pops (restore) the
   * matrix */
  

  gl4duPushMatrix(); {
    gl4duTranslatef(_cam.x, 0.0, _cam.z);
    gl4duRotatef(-90, 1, 0, 0);
    gl4duScalef(_planeScale, _planeScale, 1);
    gl4duSendMatrices();
  } gl4duPopMatrix();



  /* culls the back faces */
  glCullFace(GL_BACK);
  /* uses the checkboard texture */
  glBindTexture(GL_TEXTURE_2D, _planeTexId);
  /* sets in pId the uniform variable texRepeat to the plane scale */
  glUniform1f(glGetUniformLocation(_pId, "texRepeat"), _planeScale);
  /* tells pId that the sky is false */
  glUniform1i(glGetUniformLocation(_pId, "sky"), 0);
  cam[0] = _cam.x / (2.0 * _planeScale);
  cam[1] = -_cam.z / (2.0 * _planeScale);
  glUniform2fv(glGetUniformLocation(_pId, "cam"), 1, cam);
  /* draws the plane */
  gl4dgDraw(_plane);

  /*placement des plantes dans la map*/

  for (int i = 0; i < nb_plantes; ++i){
        unObjetplante(random_range(-70,70),random_range(-70,70),0);
  }




  cam[0] = 0;
  cam[1] = 0;
  glUniform2fv(glGetUniformLocation(_pId, "cam"), 1, cam);

  /* pushs (saves) the current matrix (modelViewMatrix), scales,
   * translates, sends matrices to pId and then pops (restore) the
   * matrix */
  /* this part means that the skydome always follow the camera position */
 



  gl4duPushMatrix(); {
    gl4duTranslatef(_cam.x, 1.0, _cam.z);
    gl4duScalef(_planeScale, _planeScale, _planeScale);
    gl4duSendMatrices();
  } gl4duPopMatrix(); 
 

  glCullFace(GL_FRONT);

  glBindTexture(GL_TEXTURE_2D, _skyTexId);

  glUniform1f(glGetUniformLocation(_pId, "texRepeat"), 1);

  glUniform1i(glGetUniformLocation(_pId, "sky"), 1);

 
  gl4dgDraw(_sphere);

 

  mat = gl4duGetMatrixData();
  MMAT4XVEC4(lumPos, mat, _lumPos0);
  glUniform4fv(glGetUniformLocation(_pId, "lumPos"), 0, lumPos);
  glUniform1i(glGetUniformLocation(_pId, "specular"), 1);

  



srand(1000);
int i;
  for (i = 0; i < nb; ++i){
     fixedObjet(100.0 * (2.0 * myRand() - 1.0),600.0 * (2.0 * myRand() - 1.0),0);
     fixedObjet(100.0 * (2.0 * myRand() - 1.0),600.0 * (2.0 * myRand() - 1.0),1);
     fixedObjet(100.0 * (2.0 * myRand() - 1.0),600.0 * (2.0 * myRand() - 1.0),2);
     fixedObjet(100.0 * (2.0 * myRand() - 1.0),600.0 * (2.0 * myRand() - 1.0),3);

   } 


  for (i = 0; i < nbanimal; ++i){

      if (animal[i].type==0) 
           movingObjet(animal[i].x,animal[i].z,0);
          
      
      if (animal[i].type==1) 
           movingObjet(animal[i].x,animal[i].z,1);
       


     if (animal[i].type==2)  
            movingObjet(animal[i].x,animal[i].z,2);


     if (animal[i].type==3)  
            movingObjet(animal[i].x,animal[i].z,3);

     if (animal[i].type==4)  
            movingObjet(animal[i].x,animal[i].z,4);

     if (animal[i].type==5)  
            movingObjet(animal[i].x,animal[i].z,5);

 	
  }

      movingCollision();
      fixedCollision();

      movedauphin(dt);
      movegoldfish(dt);
      movetortue(dt);
      movehippocampe(dt);
      movemeduse(dt);
      movebulle(dt);


}




/*!\brief function called by GL4Dummies' loop at idle.
 * 
 * uses the virtual keyboard states to move the camera according to
 * direction, orientation and time (dt = delta-time)
 */
static void idle(void) {

  double dt, dtheta = M_PI, step = 25.0;
  static Uint32 t0 = 0, t;
  dt = ((t = SDL_GetTicks()) - t0) / 1000.0;
  t0 = t;
  //dt = 1.0 / 60.0;
  if(_keys[KLEFT])
    _cam.theta += dt * dtheta;
  if(_keys[KRIGHT])
    _cam.theta -= dt * dtheta;
  if(_keys[KUP]) {
    _cam.x += -dt * step * sin(_cam.theta);
    _cam.z += -dt * step * cos(_cam.theta);
  }
  if(_keys[KDOWN]) {
    _cam.x += dt * step * sin(_cam.theta);
    _cam.z += dt * step * cos(_cam.theta);
  }

  
}

int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "Le Monde de Nemo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         _windowWidth, _windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN))
    return 1;
  initGL();
  initData();
  assimpInit("Palm_01.obj");
  atexit(quit);
  fixedElements();
  movingElements(); 

  gl4duwResizeFunc(resize);
  gl4duwKeyUpFunc(keyup);
  gl4duwKeyDownFunc(keydown);
  gl4duwDisplayFunc(draw);
  gl4duwIdleFunc(idle);
  gl4duwMainLoop();
  return 0;
}

/*!\brief creates the window, initializes OpenGL parameters,
 * initializes data and maps callback functions */

/*!\brief function called at exit. Frees used textures and clean-up
 * GL4Dummies.*/
static void quit(void) {
  

  if(_planeTexId)
    glDeleteTextures(1, &_planeTexId);
  if(_skyTexId)
    glDeleteTextures(1, &_skyTexId);
  gl4duClean(GL4DU_ALL);
}
