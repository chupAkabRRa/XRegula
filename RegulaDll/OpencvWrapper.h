#ifndef _FACE_RECOGNITION_H_
#define _FACE_RECOGNITION_H_

#include <vector>
#include <string>
#include <mutex>

#include <boost\filesystem.hpp>

namespace cv
{
	class Mat;
	template<typename _Tp> class Rect_;
	typedef Rect_<int> Rect2i;
	typedef Rect2i Rect;
}

class OpencvWrapper
{
public:
    enum class DetectionType {
        FACE = 0,
        EYES,
        MOUTH
    };
    struct  DetectionResult {
        DetectionType type;
        uint32_t x1, y1;
        uint32_t width, height;
    };

    static bool DetectFacialFeatures(const boost::filesystem::path& strPathToFile,
        std::vector<DetectionResult>& vResults);

    static bool SaveResults(const boost::filesystem::path& strPathToFile,
        std::vector<DetectionResult>& vResults);

private:
	static bool DetectFaces(cv::Mat& img, std::vector<cv::Rect>& faces);
	static bool DetectEyes(cv::Mat& img, std::vector<cv::Rect>& eyes);
	static bool DetectMouth(cv::Mat& img, std::vector<cv::Rect>& mouths);

    static std::mutex m_mtxOutput;
    static std::mutex m_mtxJson;

	static std::string m_strFaceCascade, m_strEyeCascade, m_strMouthCascade;
};

#endif // _FACE_RECOGNITION_H_
