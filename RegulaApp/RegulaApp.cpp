#include "LibraryInterfaceWin.h"

#include <iostream>
#include <boost\filesystem.hpp>

#define DEFAULT_NUM_OF_WORKERS 8

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cout << "Usage: " << argv[0] << " path_to_dir [num_of_workers]" << std::endl;
		return 1;
	}

	std::unique_ptr<LibraryInterface> lib = std::make_unique<LibraryInterfaceWin>();
	if (lib)
	{
		int res;
		res = lib->LoadRegulaLibrary(L"RegulaDll.dll");
		if (res)
		{
			std::cout << "ERROR: Can't load RegulaDll.dll: " << res << std::endl;
			return res;
		}

		size_t numOfWorkers = (argc > 2) ? std::stoi(argv[2]) : 8;
		bool bRes;
		bRes = lib->_CreateThreadPool(numOfWorkers);
		if (!bRes)
		{
			std::cout << "ERROR: Can't create ThreadPool" << std::endl;
			return 1;
		}

		boost::filesystem::path pathToDir(argv[1]);
		if (boost::filesystem::exists(pathToDir))
		{
			boost::filesystem::recursive_directory_iterator dir(pathToDir), end;

			while (dir != end)
			{
				lib->_EnqueueImage(dir->path());
				dir++;
			}
		}
		else
		{
			std::cout << "ERROR: Path doesn't exist" << std::endl;
			return 1;
		}

		lib->_DestroyThreadPool();
	}

    return 0;
}

