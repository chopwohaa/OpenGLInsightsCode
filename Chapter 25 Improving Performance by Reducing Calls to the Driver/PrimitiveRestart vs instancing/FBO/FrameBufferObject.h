
/**********************************************************************\
* AUTHOR : HILLAIRE Sébastien
*
* MAIL   : hillaire_sebastien@yahoo.fr
* SITE   : sebastien.hillaire.free.fr
*
*	You are free to totally or partially use this file/code.
* If you do, please credit me in your software or demo and leave this
* note.
*	Share your work and your ideas as much as possible!
\*********************************************************************/


#ifndef _FRAME_BUFFER_OBJECT_OPENGL_
#define _FRAME_BUFFER_OBJECT_OPENGL_


#include "stdlib.h"
#include "assert.h"
#include "stdio.h"
#include "GL/glew.h"

typedef enum{
	FBO_DepthBufferType_NONE=0,
	FBO_DepthBufferType_RENDERTARGET=1,						//24bits depth buffer
	FBO_DepthBufferType_TEXTURE=2,							//24bits depth buffer
	FBO_DepthBufferType_TEXTURE_PACKED_DEPTH24_STENCIL8=3,	//24bits depth buffer + packed 8bits stencil (32bits per pixel)
	FBO_DepthBufferType_TEXTURE_DEPTH32F=4,					//32bits floating point depth buffer
	FBO_DepthBufferType_TEXTURE_PACKED_DEPTH32F_STENCIL8=5,	//32bits floating point depth buffer + packed 8bits stencil (64bits per pixel)
	FBO_DepthBufferType_TEXTURE_FROM_ANOTHER_FBO=6			//link to this FBO the same dept buffer as another FBO passed in as a parameter
} FBO_DepthBufferType;

/**
 *	The FrameBufferObject class allowing to use render to texture
 */
class FrameBufferObject
{
private:
	/**
	 *	The OpenGL FBO ID
	 */
    GLuint fbo;

	/**
	 *	the table containing all texture id of each render target
	 */
	GLuint* colorTextures;
	/**
	 *	the number of color attachement
	 */
	GLint nbColorAttachement;
	/**
	 *	The minification filtering for the color buffer textures
	 */
	GLuint *colorMinificationFiltering;

	/**
	 *	the depth texture/buffer id
	 */
	GLuint depthID;
	/**
	 *	the depth type
	 */
	FBO_DepthBufferType depthType;
	/**
	 *	The minification filtering for the depth buffer texture
	 */
	GLuint depthMinificationFiltering;

	/**
	 * the width in pixel
	 */
	unsigned int width;
	/**
	 * the height in pixel
	 */
	unsigned int height;

	/**
	 * indicate if the mipmap pyramid correspond to the color image (last render to texture were followed by a generateMipMap)
	 */
	mutable bool* validColorMipMapPyramid;
	/**
	 * indicate if the mipmap pyramid correspond to the depth image (last render to texture were followed by a generateMipMap)
	 */
	mutable bool validDepthMipMapPyramid;

	/**
	 *	default constructeur not allowed
	 */
	FrameBufferObject();
	/**
	 *	copy constructeur not allowed
	 */
	FrameBufferObject(FrameBufferObject&);
	/**
	 * copy assignement operator not allowed
	 */
	FrameBufferObject& operator=(const FrameBufferObject&);

public:

	/**
	 *	The only available constructor
	 */
	FrameBufferObject(unsigned int width, unsigned int height,
		unsigned int nbColorBuffer, 
		const unsigned int* colorBufferInternalFormat, 
		const unsigned int* colorBufferSWRAP,
		const unsigned int* colorBufferTWRAP,
		const unsigned int* colorBufferMinFiltering,
		const unsigned int* colorBufferMagFiltering,
		FBO_DepthBufferType depthBufferType=FBO_DepthBufferType_NONE,
		const unsigned int depthBufferMinFiltering=GL_LINEAR,
		const unsigned int depthBufferMagFiltering=GL_LINEAR,
		const unsigned int depthBufferSWRAP=GL_CLAMP,
		const unsigned int depthBufferTWRAP=GL_CLAMP,
		bool depthTextureCompareToR=false,
		FrameBufferObject* fboContainingDepthBuffer=NULL);

	/**
	 *	The destructor
	 */
	~FrameBufferObject();

	/**
	 * @return the number of color attachment of this FBO
	 */
	unsigned int getNumberOfColorAttachement() const;
	/**
	 * @return the type of depth buffer of this FBO
	 */
	FBO_DepthBufferType getDepthBufferType() const;

	/**
	 * @return the width in pixel of this FBO
	 */
	unsigned int getWidth() const;
	/**
	 * @return the height in pixel of this FBO
	 */
	unsigned int getHeight() const;

	/**
	 *	@return the maximum number of color texture attachement
	 */
	static const GLint getMaxColorAttachments();
	/**
	 *	@return the maximum width and height allowed
	 */
	static const GLint getMaxBufferSize();

	/**
	 *	Enable this FBO to render in ONE texture using the depth buffer if it exist.
	 * To disable rendering in the depth buffer and/or depth testing, use glDepthMask and glDisable(GL_DEPTH_TEST).
	 *
	 * @param colorBufferNum : the number of the render texture (0 for GL_COLOR_ATTACHMENT0_EXT, etc)
	 */
	void enableRenderToColorAndDepth(const unsigned int colorBufferNum) const;
	/**
	 *	Enable this FBO to render in MULTIPLE texture using the depth buffer if it exist.
	 * To disable rendering in the depth buffer and/or depth testing, use glDepthMask and glDisable(GL_DEPTH_TEST).
	 *	Using this method, you must not use gl_fragColor in the fragment program but gl_fragData[i].
	 *
	 * @param numBuffers : the number of renger target we want
	 * @param drawbuffers : an array containing the color texture ID binded to the fragment program output gl_fragData[i]
	 */
	void FrameBufferObject::enableRenderToColorAndDepth_MRT(const GLuint numBuffers, const GLenum* drawbuffers) const;
	/**
	 *	Disable render to texture. This method os static because it works the same way for all fbo instance.
	 * Thus, if you want to render to two render texture sequentially, you don't need to disable fbo rendering
	 * and it will improves performances.
	 */
	static void disableRenderToColorDepth();

	/**
	 *	Save the current viewport and set up the viewport for this fbo. If the FBO is has same resolution as the current
	 * viewport, you don't need to call this method.
	 */
	void saveAndSetViewPort() const;
	/**
	 *	Restore previous viewport.
	 * @see saveAndSetViewPort
	 */
	void restoreViewPort() const;

	/**
	 *	Bind a color buffer in the current texture unit as a GL_TEXTURE_2D.
	 *
	 * @param colorBufferNum : the number of the color buffer texture to bind. If the number is invalid, texture 0 is binded
	 */
	void bindColorTexture(const unsigned int colorBufferNum) const;
	/**
	 *	Bind the depth buffer in the current texture unit as a GL_TEXTURE_2D.
	 */
	void bindDepthTexture() const;

	/**
	 *	Generate the mipmap chain for a given color buffer.
	 * The mipmap generation will not work for texture-buffer you have specified with GL_NEAREST or GL_LINEAR at the creation.
	 *
	 * @param colorBufferNum : the number of the color buffer texture to bind. If the number is invalid, we return.
	 */
	void generateColorBufferMipMap(const unsigned int colorBufferNum) const;
	/**
	 *	Generate the mipmap chain for the depth buffer. Some driver don't support it.
	 */
	void generateDepthBufferMipMap() const;
	/**
	 *
	 */
	bool isColorMipMapPyramidValid(const unsigned int colorBufferNum) const;
	/**
	 *
	 */
	bool isDepthMipMapPyramidValid() const;

	/**
	 *	Save a given color bufferin a bitmap file.
	 *
	 * @param colorBufferNum : the number of the color buffer texture to bind. If the number is invalid, we return.
	 * @param filepath : the path of the bitmap file.
	 */
	void saveToDisk(const unsigned int colorBufferNum, const char* filepath) const;

	bool isFrameBufferComplete();

};



#endif

