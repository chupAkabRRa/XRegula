#include "OpencvWrapper.h"

#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <iostream>
#include <vector>
#include <fstream>

// static
std::mutex OpencvWrapper::m_mtxOutput;
std::mutex OpencvWrapper::m_mtxJson;
std::string OpencvWrapper::m_strFaceCascade = "..\\..\\..\\cascades\\haarcascade_frontalface_default.xml"; 
std::string OpencvWrapper::m_strEyeCascade = "..\\..\\..\\cascades\\haarcascade_eye.xml";
std::string OpencvWrapper::m_strMouthCascade = "..\\..\\..\\cascades\\haarcascade_mcs_mouth.xml";

// static
bool OpencvWrapper::DetectFacialFeatures(const boost::filesystem::path& strPathToFile,
    std::vector<DetectionResult>& vResults)
{
	std::vector<cv::Rect> vFaces;
	bool bRes;
	cv::Mat image = cv::imread(strPathToFile.string());

	if (image.data == nullptr)
	{
		std::unique_lock<std::mutex> lck(m_mtxOutput);
		std::cout << "OpencvWrapper::DetectFacialFeatures: Can't read file " << strPathToFile << std::endl;
		return false;
	}

	bRes = DetectFaces(image, vFaces);
	if (bRes)
	{
		{
			std::unique_lock<std::mutex> lck(m_mtxOutput);
			std::cout << strPathToFile << " -> " << vFaces.size() << " face(s) detected" << std::endl;
		}

		DetectionResult result;
		for (unsigned int i = 0; i < vFaces.size(); i++)
		{
			// Save info about the face
			result.type = DetectionType::FACE;
			result.x1 = vFaces[i].x;
			result.y1 = vFaces[i].y;
			result.width = vFaces[i].width;
			result.height = vFaces[i].height;
			vResults.push_back(result);

			// Region-Of-Interst for further analysis
			cv::Mat ROI = image(cv::Rect(result.x1, result.y1, result.width, result.height));

			// Eyes detection
			std::vector<cv::Rect> vEyes;
			bRes = DetectEyes(ROI, vEyes);
			if (bRes)
			{
				for (unsigned int j = 0; j < vEyes.size(); j++)
				{
					result.type = DetectionType::EYES;
					result.x1 = vEyes[j].x;
					result.y1 = vEyes[j].y;
					result.width = vEyes[j].width;
					result.height = vEyes[j].height;
					vResults.push_back(result);
				}
			}

			// Mouth detection
			std::vector<cv::Rect> vMouths;
			bRes = DetectMouth(ROI, vMouths);
			if (bRes)
			{
				for (unsigned int j = 0; j < vMouths.size(); j++)
				{
					result.type = DetectionType::MOUTH;
					result.x1 = vMouths[j].x;
					result.y1 = vMouths[j].y;
					result.width = vMouths[j].width;
					result.height = vMouths[j].height;
					vResults.push_back(result);
				}
			}
		}
	}
	else
	{
		std::unique_lock<std::mutex> lck(m_mtxOutput);
		std::cout << "OpencvWrapper::DetectFacialFeatures: Can't detect faces in " << strPathToFile << std::endl;
	}

    return true;
}

// static
bool OpencvWrapper::SaveResults(const boost::filesystem::path& strPathToFile,
    std::vector<DetectionResult>& vResults)
{
	boost::filesystem::path pathSavePattern(strPathToFile.parent_path());
	pathSavePattern /= strPathToFile.stem();
	int FaceCounter = 0;
	std::string strJsonElement;
	
	cv::Mat originalImage = cv::imread(strPathToFile.string());
	if (originalImage.data == nullptr)
	{
		return false;
	}

	for (unsigned int i = 0; i < vResults.size(); i++)
	{
		if (vResults[i].type == DetectionType::FACE)
		{
			cv::Mat croppedImage;
			cv::Rect faceRect(vResults[i].x1, vResults[i].y1, vResults[i].width, vResults[i].height);
			originalImage(faceRect).copyTo(croppedImage);
			cv::flip(croppedImage, croppedImage, -1);

			std::string strFinalFileName = pathSavePattern.string() + "_face" + std::to_string(FaceCounter++) + ".jpg";
			cv::imwrite(strFinalFileName, croppedImage);

			// Formatting JSON unit to add to file later
			strJsonElement += "{\n\t\"filename\": \"" + strFinalFileName + "\",\n";
			strJsonElement += "\t\"coordinates\": [\n\t\t[" + std::to_string(vResults[i].x1) +
				", " + std::to_string(vResults[i].y1) + "],\n\t\t[" + std::to_string(vResults[i].x1 + vResults[i].width) +
				", " + std::to_string(vResults[i].y1 + vResults[i].height) + "]\n\t]\n},\n";
		}
	}

	// Remove trailing ",\n" symbols
	strJsonElement.pop_back();
	strJsonElement.pop_back();

	// JSON work is under the lock since couple of threads can process files in the same folder,
	// so we need sync here
	{
		std::unique_lock<std::mutex> lck(m_mtxJson);
		boost::filesystem::path pathJson(strPathToFile.parent_path());
		pathJson /= "result.json";

		if (boost::filesystem::exists(pathJson))
		{
			std::fstream fileJson(pathJson.string());
			if (fileJson.is_open())
			{
				// Just insert new elements in the existing array (i.e. skip one 
				// last symbol in file which is ']')
				fileJson.seekp(-1, std::ios::end);
				strJsonElement = ",\n" + strJsonElement + "]";
				fileJson << strJsonElement;
				fileJson.close();
			}
		}
		else
		{
			std::fstream fileJson(pathJson.string(), std::fstream::out);
			if (fileJson.is_open())
			{
				// Since it's initial insertion - create a JSON array element
				strJsonElement = "[" + strJsonElement + "]";
				fileJson << strJsonElement;
				fileJson.close();
			}
		}
	}

    return true;
}

// static
bool OpencvWrapper::DetectFaces(cv::Mat& img, std::vector<cv::Rect>& faces)
{
	cv::CascadeClassifier FaceCascade;
	bool bRes = FaceCascade.load(m_strFaceCascade);

	if (bRes)
	{
		FaceCascade.detectMultiScale(img, faces, 1.15, 3, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
	}

	return bRes;
}

// static
bool OpencvWrapper::DetectEyes(cv::Mat& img, std::vector<cv::Rect>& eyes)
{
	cv::CascadeClassifier EyesCascade;
	bool bRes = EyesCascade.load(m_strEyeCascade);

	if (bRes)
	{
		EyesCascade.detectMultiScale(img, eyes, 1.20, 5, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
	}

	return bRes;
}

// static
bool OpencvWrapper::DetectMouth(cv::Mat& img, std::vector<cv::Rect>& mouths)
{
	cv::CascadeClassifier MouthCascade;
	bool bRes = MouthCascade.load(m_strEyeCascade);

	if (bRes)
	{
		MouthCascade.detectMultiScale(img, mouths, 1.20, 5, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));
	}

	return bRes;
}