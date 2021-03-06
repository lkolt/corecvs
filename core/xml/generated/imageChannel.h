#ifndef IMAGECHANNEL_H_
#define IMAGECHANNEL_H_
/**
 * \file imageChannel.h
 * \attention This file is automatically generated and should not be in general modified manually
 *
 * \date MMM DD, 20YY
 * \author autoGenerator
 */

/**
 * Helper namespace to hide ImageChannel enum from global context.
 */

namespace ImageChannel {

/** 
 * \brief ImageChannel 
 * ImageChannel 
 */
enum ImageChannel {
    /** 
     * \brief R 
     * R 
     */
    R = 0,
    /** 
     * \brief G 
     * G 
     */
    G = 1,
    /** 
     * \brief B 
     * B 
     */
    B = 2,
    /** 
     * \brief Alpha 
     * Alpha 
     */
    ALPHA = 3,
    /** 
     * \brief Y 
     * Y 
     */
    Y = 4,
    /** 
     * \brief Cr 
     * Cr 
     */
    CR = 5,
    /** 
     * \brief Cb 
     * Cb 
     */
    CB = 6,
    /** 
     * \brief U 
     * U 
     */
    U = 7,
    /** 
     * \brief V 
     * V 
     */
    V = 8,
    /** 
     * \brief Chroma 
     * Chroma 
     */
    CHROMA = 9,
    /** 
     * \brief Gray 
     * Gray 
     */
    GRAY = 10,
    /** 
     * \brief Luma 
     * Luma 
     */
    LUMA = 11,
    /** 
     * \brief Hue 
     * Hue 
     */
    HUE = 12,
    /** 
     * \brief Saturation 
     * Saturation 
     */
    SATURATION = 13,
    /** 
     * \brief Value 
     * Value 
     */
    VALUE = 14,
    /** 
     * \brief Last virtual option to run cycles to
     */
    IMAGECHANNEL_LAST
};


static inline const char *getName(const ImageChannel &value)
{
    switch (value) 
    {
     case R : return "R"; break ;
     case G : return "G"; break ;
     case B : return "B"; break ;
     case ALPHA : return "ALPHA"; break ;
     case Y : return "Y"; break ;
     case CR : return "CR"; break ;
     case CB : return "CB"; break ;
     case U : return "U"; break ;
     case V : return "V"; break ;
     case CHROMA : return "CHROMA"; break ;
     case GRAY : return "GRAY"; break ;
     case LUMA : return "LUMA"; break ;
     case HUE : return "HUE"; break ;
     case SATURATION : return "SATURATION"; break ;
     case VALUE : return "VALUE"; break ;
     default : return "Not in range"; break ;
     
    }
    return "Not in range";
}

} //namespace ImageChannel

#endif  //IMAGECHANNEL_H_
