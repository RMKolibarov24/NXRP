#include "pch.h"
#include "CppUnitTest.h"
#include "Maze.Settings.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace MazeUnitTest
{
    namespace
    {
        class SettingsDirectory
        {
        public:
            SettingsDirectory()
            {
                static std::atomic<unsigned int> counter{ 0 };
                const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
                directory_ = std::filesystem::temp_directory_path()
                    / ("NXRP-settings-tests-" + std::to_string(stamp)
                        + "-" + std::to_string(counter.fetch_add(1)));
                // Clean up only a directory that this fixture actually created.
                if (!std::filesystem::create_directory(directory_))
                    throw std::runtime_error("Could not create an isolated settings-test directory.");
            }

            ~SettingsDirectory()
            {
                std::error_code ignored;
                std::filesystem::remove_all(directory_, ignored);
            }

            SettingsDirectory(const SettingsDirectory&) = delete;
            SettingsDirectory& operator=(const SettingsDirectory&) = delete;

            std::filesystem::path file() const { return directory_ / "settings.txt"; }

        private:
            std::filesystem::path directory_;
        };

        void WriteText(const std::filesystem::path& path, const std::string& text)
        {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            file << text;
            file.close();
            Assert::IsTrue(static_cast<bool>(file), L"Could not write test input.");
        }

        std::string ReadText(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
        }

        void AssertSize(const MazeSize& actual, int rows, int cols)
        {
            Assert::AreEqual(rows, actual.rows);
            Assert::AreEqual(cols, actual.cols);
        }
    }

    TEST_CLASS(MazeSettingsTests)
    {
    public:
        TEST_METHOD(MissingFileUsesDefaultsWithoutWriting)
        {
            SettingsDirectory directory;
            AssertSize(loadMazeSizePreferences(directory.file()), 10, 10);
            Assert::IsFalse(std::filesystem::exists(directory.file()));
        }

        TEST_METHOD(LoadsRectanglesAndWhitespace)
        {
            SettingsDirectory directory;
            WriteText(directory.file(), " \r\n12\t37\r\n ");
            AssertSize(loadMazeSizePreferences(directory.file()), 12, 37);
            WriteText(directory.file(), "37 12");
            AssertSize(loadMazeSizePreferences(directory.file()), 37, 12);
        }

        TEST_METHOD(LoadsBoundarySizes)
        {
            SettingsDirectory directory;
            for (const auto size : { MazeSize(10, 10), MazeSize(10, 50), MazeSize(50, 10), MazeSize(50, 50) })
            {
                WriteText(directory.file(), std::to_string(size.rows) + " " + std::to_string(size.cols));
                AssertSize(loadMazeSizePreferences(directory.file()), size.rows, size.cols);
            }
        }

        TEST_METHOD(InvalidDimensionsUseDefaults)
        {
            SettingsDirectory directory;
            for (const auto input : { "9 20", "20 9", "51 20", "20 51", "0 0", "-10 20", "20 -10" })
            {
                WriteText(directory.file(), input);
                AssertSize(loadMazeSizePreferences(directory.file()), 10, 10);
            }
        }

        TEST_METHOD(MalformedOrExtraDataUsesDefaults)
        {
            SettingsDirectory directory;
            for (const auto input : { "", " ", "20", "hello", "20 columns", "20 20x", "20 20 30",
                "20.5 20", "999999999999999999999999 20", "20 999999999999999999999999", "20 20\x1a extra" })
            {
                WriteText(directory.file(), input);
                AssertSize(loadMazeSizePreferences(directory.file()), 10, 10);
            }
        }

        TEST_METHOD(SavesAndCreatesParentDirectories)
        {
            SettingsDirectory directory;
            const auto file = directory.file().parent_path() / L"размери" / "nested" / "settings.txt";
            Assert::IsTrue(saveMazeSizePreferences(file, { 12, 37 }));
            AssertSize(loadMazeSizePreferences(file), 12, 37);
            Assert::IsTrue(ReadText(file) == "12 37\r\n" || ReadText(file) == "12 37\n");
            auto temporary = file;
            temporary += ".tmp";
            Assert::IsFalse(std::filesystem::exists(temporary));
        }

        TEST_METHOD(SavingReplacesExistingPreferences)
        {
            SettingsDirectory directory;
            Assert::IsTrue(saveMazeSizePreferences(directory.file(), { 10, 50 }));
            Assert::IsTrue(saveMazeSizePreferences(directory.file(), { 50, 10 }));
            AssertSize(loadMazeSizePreferences(directory.file()), 50, 10);
        }

        TEST_METHOD(InvalidSavePreservesExistingPreferences)
        {
            SettingsDirectory directory;
            WriteText(directory.file(), "12 37\n");
            const auto before = ReadText(directory.file());
            for (const auto size : { MazeSize(9, 20), MazeSize(20, 51), MazeSize(-1, 20) })
            {
                Assert::IsFalse(saveMazeSizePreferences(directory.file(), size));
                Assert::IsTrue(ReadText(directory.file()) == before);
            }
            const auto absent = directory.file().parent_path() / "must-not-create" / "settings.txt";
            Assert::IsFalse(saveMazeSizePreferences(absent, { 51, 20 }));
            Assert::IsFalse(std::filesystem::exists(absent.parent_path()));
        }

        TEST_METHOD(EmptyPathUsesDefaultsAndCannotSave)
        {
            AssertSize(loadMazeSizePreferences({}), 10, 10);
            Assert::IsFalse(saveMazeSizePreferences({}, { 12, 37 }));
        }

        TEST_METHOD(UnreadableFileUsesDefaults)
        {
            SettingsDirectory directory;
            std::filesystem::create_directory(directory.file());
            AssertSize(loadMazeSizePreferences(directory.file()), 10, 10);
        }

        TEST_METHOD(SaveFailsWhenParentIsAFile)
        {
            SettingsDirectory directory;
            WriteText(directory.file(), "keep this file");
            Assert::IsFalse(saveMazeSizePreferences(directory.file() / "child.txt", { 12, 37 }));
            Assert::IsTrue(ReadText(directory.file()) == "keep this file");
        }

        TEST_METHOD(FailedReplacePreservesDestination)
        {
            SettingsDirectory directory;
            std::filesystem::create_directory(directory.file());
            const auto marker = directory.file() / "keep.txt";
            WriteText(marker, "keep this directory");
            Assert::IsFalse(saveMazeSizePreferences(directory.file(), { 12, 37 }));
            Assert::IsTrue(ReadText(marker) == "keep this directory");
            auto temporary = directory.file();
            temporary += ".tmp";
            Assert::IsFalse(std::filesystem::exists(temporary));
        }
    };
}
