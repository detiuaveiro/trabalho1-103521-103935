/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec: 103521  Name: Diogo Tavares de Vasconcelos Aguiar Soares
// NMec: 103935  Name: José Pedro Tavares Lopes
//
//
//
// Date: 26/11/2023
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
//
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image
{
  int width;
  int height;
  int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
  uint8 *pixel; // pixel data (a raster scan)
};

// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
//
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char *errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char *ImageErrMsg()
{ ///
  return errCause;
}

// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success =
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
//
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
//
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
//
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)

// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char *failmsg)
{
  errCause = (char *)(condition ? "" : failmsg);
  return condition;
}

/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void)
{ ///
  InstrCalibrate();
  InstrName[0] = "pixmem"; // InstrCount[0] will count pixel array acesses
  // Name other counters here...
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...

// TIP: Search for PIXMEM or InstrCount to see where it is incremented!

/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval)
{ ///
  assert(width >= 0);
  assert(height >= 0);
  assert(0 < maxval && maxval <= PixMax);
  // Insert your code here!

  Image createdImage = malloc(sizeof(struct image)); // alocação de espaço para a nova imagem

  if (createdImage == NULL)
  { // erro na alocação de memória

    errCause = "Não foi possível alocar memória para nova imagem";
    errno = 12; // número 12 para errno significa falha de alocação de memória
    return NULL;
  }

  // Fornecer à nova imagem os valores usados na chamada da imagem
  createdImage->width = width;
  createdImage->height = height;
  createdImage->maxval = maxval;

  // Usando o calloc a memória será inicializada com o valor 0
  createdImage->pixel = calloc(width * height, sizeof(uint8));

  if (createdImage->pixel == NULL) // Se houver erros na alocação de memória para os pixeis
  {
    free(createdImage); // liberta memória usada
    errCause = "Não foi possível alocar memória para os pixeis da nova imagem";
    errno = 12; // número 12 para errno significa falha de alocação de memória
    return NULL;
  }

  return createdImage;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image *imgp)
{ ///
  assert(imgp != NULL);
  // Insert your code here!

  free((*imgp)->pixel); // libertar memória alocada para o array pixel de imgp
  free(*imgp);          // libertar memória associada com imgp
  *imgp = NULL;         // faz com que o ponteiro para imgp se torne NULL por razões de segurança
}

/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE *f)
{
  char c;
  int i = 0;
  while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n')
  {
    i++;
  }
  return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char *filename)
{ ///
  int w, h;
  int maxval;
  char c;
  FILE *f = NULL;
  Image img = NULL;

  int success =
      check((f = fopen(filename, "rb")) != NULL, "Open failed") &&
      // Parse PGM header
      check(fscanf(f, "P%c ", &c) == 1 && c == '5', "Invalid file format") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d ", &h) == 1 && h >= 0, "Invalid height") &&
      skipComments(f) >= 0 &&
      check(fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int)PixMax, "Invalid maxval") &&
      check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected") &&
      // Allocate image
      (img = ImageCreate(w, h, (uint8)maxval)) != NULL &&
      // Read pixels
      check(fread(img->pixel, sizeof(uint8), w * h, f) == w * h, "Reading pixels");
  PIXMEM += (unsigned long)(w * h); // count pixel memory accesses

  // Cleanup
  if (!success)
  {
    errsave = errno;
    ImageDestroy(&img);
    errno = errsave;
  }
  if (f != NULL)
    fclose(f);
  return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char *filename)
{ ///
  assert(img != NULL);
  int w = img->width;
  int h = img->height;
  uint8 maxval = img->maxval;
  FILE *f = NULL;

  int success =
      check((f = fopen(filename, "wb")) != NULL, "Open failed") &&
      check(fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed") &&
      check(fwrite(img->pixel, sizeof(uint8), w * h, f) == w * h, "Writing pixels failed");
  PIXMEM += (unsigned long)(w * h); // count pixel memory accesses

  // Cleanup
  if (f != NULL)
    fclose(f);
  return success;
}

/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img)
{ ///
  assert(img != NULL);
  return img->width;
}

/// Get image height
int ImageHeight(Image img)
{ ///
  assert(img != NULL);
  return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img)
{ ///
  assert(img != NULL);
  return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8 *min, uint8 *max)
{ ///
  assert(img != NULL);
  // Insert your code here!

  uint8 *maxpixel, *minpixel; // criar ponteiros para guardar o pixel com valor máximo e minimo da imagem
  // iniciar ponteiros
  maxpixel = &(img->pixel[0]);
  minpixel = &(img->pixel[0]);

  for (int i = 1; i < img->width * img->height; i++)
  { // percorrer o array de pixeis
    if (*maxpixel < img->pixel[i])
      maxpixel = &(img->pixel[i]); // fornecer novo valor ao maxpixel

    if (*minpixel > img->pixel[i])
      minpixel = &(img->pixel[i]); // fornecer novo valor ao minpixel
  }

  *min = *minpixel;
  *max = *maxpixel;
}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y)
{ ///
  assert(img != NULL);
  return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h)
{ ///
  assert(img != NULL);
  // Insert your code here!

  int rectwidthpos = x + w;  // localização da posição em x máxima
  int rectheightpos = y + h; // localização da posição em y máxima

  if (rectheightpos > img->height || rectwidthpos > img->width)
  {

    errCause = "O retângulo é inválido pois está fora dos limites da imagem";
    errno = 22; // número 22 para errno significa que o argumento para a função é inválido;
    return -1;
  }

  return 1;
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel.
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y)
{
  int index;
  // Insert your code here!

  int imgwidth = img->width; // tamanho de uma linha
  index = imgwidth * y + x;  // index é o valor se transformadas as coordenadas para um array

  assert(0 <= index && index < img->width * img->height); // verificar que o index se encontra dentro dos limites
  return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y)
{ ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1; // count one pixel access (read)
  return img->pixel[G(img, x, y)];
}

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level)
{ ///
  assert(img != NULL);
  assert(ImageValidPos(img, x, y));
  PIXMEM += 1; // count one pixel access (store)
  img->pixel[G(img, x, y)] = level;
}

/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.

/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img)
{ ///
  assert(img != NULL);
  // Insert your code here!

  for (int i = 0; i < img->width * img->height; i++) // percorrer array de pixeis
    img->pixel[i] = img->maxval - img->pixel[i];     // modificação do pixel i para o seu negativo
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr)
{ ///
  assert(img != NULL);
  // Insert your code here!

  for (int i = 0; i < img->width * img->height; i++)
  {                          // percorrer array de pixeis //sizeof(img->pixel) fornece o tamanho do array em bytes logo é melhor utilizar isto
    if (img->pixel[i] < thr) // valor do pixel não chegou ao limite
      img->pixel[i] = 0;     // então dá-lhes o valor de 0 para que fiquem pretos
    else
      img->pixel[i] = img->maxval; // caso contrário dá lhes o valor máximo permitido pela img
  }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor)
{ ///
  assert(img != NULL);
  // ? assert (factor >= 0.0);
  // Insert your code here!

  assert(factor >= 0.0);

  for (int i = 0; i < img->width * img->height; i++)
  { // percorrer array de pixeis //size of pixel devolve o tamanho do array em bytes logo assim é melhor

    double newPixel = img->pixel[i] * factor; // desnecessário mas melhor para leitura, dentro do ciclo for pois é desnecessário fora

    if (newPixel > img->maxval)    // novo valor é maior que maxval
      img->pixel[i] = img->maxval; // então o novo valor fica maxval

    else // valor multiplicado normalmente, soma com 0.5 para evitar erros de arredondamento
      img->pixel[i] = (uint8)(newPixel + 0.5);
  }
}

/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint:
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees anti-clockwise.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img)
{ ///
  assert(img != NULL);
  // Insert your code here!

  Image newImg = ImageCreate(img->height, img->width, img->maxval); // alocação de espaço para nova imagem

  for (int x = 0; x < img->width; x++)    // percorrer todo x
    for (int y = 0; y < img->height; y++) // para todo x percorrer todo y
    {

      ImageSetPixel(newImg, y, img->width - x - 1, ImageGetPixel(img, x, y)); // rodar imagem 90 graus anti-clockwise
    }

  return newImg;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img)
{ ///
  assert(img != NULL);
  // Insert your code here!

  Image newImg = ImageCreate(img->width, img->height, img->maxval);
  if (newImg == NULL)
  {
    // Erro já foi feito em ImageCreate
    return NULL;
  }

  for (int x = 0; x < img->width; x++)    // percorrer todo x
    for (int y = 0; y < img->height; y++) // para x percorrer todos os y
    {

      ImageSetPixel(newImg, img->width - x - 1, y, ImageGetPixel(img, x, y));
    }

  return newImg;
}

/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h)
{ ///
  assert(img != NULL);
  assert(ImageValidRect(img, x, y, w, h));
  // Insert your code here!

  Image newImage = ImageCreate(w, h, img->maxval);

  if (newImage == NULL)
    return NULL;

  for (int i = 0; i < h; i++)                                          // variável i corresponde à coordenada y da newImage
    for (int j = 0; j < w; j++)                                        // variável j corresponde à coordenada x da new image
      ImageSetPixel(newImage, j, i, ImageGetPixel(img, x + j, x + i)); // x+j e x+i correspondem ao pixel correspondente da img

  return newImage;
}

/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));
  // Insert your code here!

  for (int i = 0; i < img2->height; i++)                            // variável i corresponde à coordenada y da newImage
    for (int j = 0; j < img2->width; j++)                           // variável j corresponde à coordenada x da newImage
      ImageSetPixel(img1, x + j, y + i, ImageGetPixel(img2, j, i)); // x+j e y+i correspondem ao pixel a ser mudado na img1
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidRect(img1, x, y, img2->width, img2->height));
  // Insert your code here!
  for (int i = 0; i < img2->height; i++)
  { // variável i corresponde à coordenada y da img2
    for (int j = 0; j < img2->width; j++)
    {                                                                                                                            // variável j corresponde à coordenada x da img2
      uint8 blendedPixel = (uint8)(alpha * ImageGetPixel(img2, j, i) + (1.0 - alpha) * ImageGetPixel(img1, x + j, y + i) + 0.5); // blend do pixel com o alpha e arredonda

      if (blendedPixel < 0)
        ImageSetPixel(img1, x + j, y + i, 0); // valor não pode ser menor que 0
      else if (blendedPixel > img1->maxval)
        ImageSetPixel(img1, x + j, y + i, img1->maxval); // valor não pode ser maior que maxval
      else
        ImageSetPixel(img1, x + j, y + i, blendedPixel);
    }
  }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  assert(ImageValidPos(img1, x, y));
  // Insert your code here!

  for (int i = 0; i < img2->height; i++)
  { // i corresponde às coordenadas y de img2
    for (int j = 0; j < img2->width; j++)
    { // j corresponde às coordenadas x de img2

      if (ImageGetPixel(img2, j, i) != ImageGetPixel(img1, x + j, y + i)) // comparar os pixeis correspondentes de cada imagem
        return 0;
    }
  }

  return 1;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int *px, int *py, Image img2)
{ ///
  assert(img1 != NULL);
  assert(img2 != NULL);
  // Insert your code here!
  for (int i = 0; i < img1->height - img2->height; i++)
  { // percorrer as linhas até altura da imagem 1 menos a altura da imagem 2
    for (int j = 0; j < img1->width - img2->width; j++)
    { // percorrer as colunas até altura da imagem 1 menos a altura da imagem 2
      if (ImageMatchSubImage(img1, j, i, img2))
      {
        *px = j;
        *py = i;
        return 1;
      }
    }
  }
  return 0;
}

/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
void ImageBlur(Image img, int dx, int dy)
{ ///
  // Insert your code here!
  assert(img != NULL);

  int ImageHeight = img->height;
    int ImageWidth = img->width;

    // Criar cópia da imagem original
    Image imgCopy = ImageCreate(ImageWidth, ImageHeight, img->maxval);
    for (int i = 0; i < ImageHeight * ImageWidth; i++) {
        imgCopy->pixel[i] = img->pixel[i];//copiar os pixeis com os valores originais
    }

    for (int i = 0; i < ImageHeight; i++) {
        for (int j = 0; j < ImageWidth; j++) {
            float sum = 0;
            int num = 0;
            //calcular a média dos pixeis vizinhos
            for (int i_height = i - dy; i_height <= i + dy; i_height++) {
                if (i_height < 0 || i_height >= ImageHeight)
                    continue;

                for (int j_width = j - dx; j_width <= j + dx; j_width++) {
                    if (j_width < 0 || j_width >= ImageWidth)
                        continue;

                    sum += ImageGetPixel(imgCopy, j_width, i_height);
                    num++;
                }
            }
            //dar o valor do pixel blurred à imagem original
            int blurredValue = (int)(((sum) / num) + 0.5);
            ImageSetPixel(img, j, i, blurredValue);
        }
    }

    ImageDestroy(&imgCopy);//libertar espaço
}

// for (int i = 0; i < img->height; i++) {
//     for (int j = 0; j < img->width; j++) {
//       float sum = 0.0;
//       int num = 0;
//
//       for (int i_height = i - dy; i_height <= i + dy; i_height++) {
//         if (i_height < 0 || i_height >= img->height)
//           continue;
//
//         for (int j_width = j - dx; j_width <= j + dx; j_width++) {
//           if (j_width < 0 || j_width >= img->width)
//             continue;
//
//           sum += ImageGetPixel(img, j_width, i_height);
//           num++;
//         }
//       }
//
//       int newPixelValue = (int)(((sum) / num) + 0.5);
//       ImageSetPixel(img, j, i, newPixelValue);
//     }
//   }

//// Aplicar blur horizontal
// for (int i = 0; i < img->height; i++) {
//   for (int j = 0; j < img->width; j++) {
//     float sum = 0;
//     int num = 0;
//
//     for (int k = j - dx; k <= j + dx; k++) { //percorrer pixeis horizontais
//       if (k >= 0 && k < img->width) {
//         sum += ImageGetPixel(img, k, i);  //sumar os valores dos pixeis
//         num++;
//       }
//     }
//
//     ImageSetPixel(img, j, i, (int)(((sum) / num) + 0.5));
//   }
// }
//
// // Aplicar blur vertical
// for (int i = 0; i < img->height; i++) {
//   for (int j = 0; j < img->width; j++) {
//     float sum = 0;
//     int num = 0;
//
//     for (int k = i - dy; k <= i + dy; k++) {  //percorrer pixeis verticais
//       if (k >= 0 && k < img->height) {
//         sum += ImageGetPixel(img, j, k);    //sumar os valores dos pixeis
//         num++;
//       }
//     }
//
//     ImageSetPixel(img, j, i, (int)(((sum) / num) + 0.5));
//   }
// }
