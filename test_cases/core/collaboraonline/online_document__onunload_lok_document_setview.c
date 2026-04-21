#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#define LOK_USE_UNSTABLE_API

#include <LibreOfficeKit/LibreOfficeKitEnums.h>
#include <LibreOfficeKit/LibreOfficeKit.hxx>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

class DesktopLOKTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(DesktopLOKTest);
    CPPUNIT_TEST(testSetView);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override;
    void tearDown() override;
    void testSetView();

private:
    lok::Office* _loKit = nullptr;
    lok::Document* _loKitDocument = nullptr;
    std::string _testDocPath;
    
    void createTestDocument();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DesktopLOKTest);

void DesktopLOKTest::createTestDocument()
{
    std::cout << "[DEBUG] createTestDocument() - START" << std::endl;
    
    // Create a minimal ODT file (OpenDocument Text)
    // ODT files are ZIP archives with specific structure
    // For simplicity, we'll create a minimal valid ODT
    
    _testDocPath = "/tmp/test_document_" + std::to_string(getpid()) + ".odt";
    std::cout << "[DEBUG] createTestDocument() - Creating test document at: " << _testDocPath << std::endl;
    
    // Create a minimal ODT structure
    // This is a base64-encoded minimal ODT file that we'll write
    // For now, let's create a simple text file and hope LOK can handle it
    // Better approach: create actual ODT structure
    
    std::string tempDir = "/tmp/odt_temp_" + std::to_string(getpid());
    std::cout << "[DEBUG] createTestDocument() - Creating temp directory: " << tempDir << std::endl;
    mkdir(tempDir.c_str(), 0755);
    
    // Create mimetype file
    std::ofstream mimeFile(tempDir + "/mimetype");
    mimeFile << "application/vnd.oasis.opendocument.text";
    mimeFile.close();
    
    // Create META-INF directory
    mkdir((tempDir + "/META-INF").c_str(), 0755);
    
    // Create manifest.xml
    std::ofstream manifestFile(tempDir + "/META-INF/manifest.xml");
    manifestFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    manifestFile << "<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\" manifest:version=\"1.2\">\n";
    manifestFile << "  <manifest:file-entry manifest:full-path=\"/\" manifest:version=\"1.2\" manifest:media-type=\"application/vnd.oasis.opendocument.text\"/>\n";
    manifestFile << "  <manifest:file-entry manifest:full-path=\"content.xml\" manifest:media-type=\"text/xml\"/>\n";
    manifestFile << "</manifest:manifest>\n";
    manifestFile.close();
    
    // Create content.xml
    std::ofstream contentFile(tempDir + "/content.xml");
    contentFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    contentFile << "<office:document-content xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" ";
    contentFile << "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\">\n";
    contentFile << "  <office:body>\n";
    contentFile << "    <office:text>\n";
    contentFile << "      <text:p>Test Document</text:p>\n";
    contentFile << "    </office:text>\n";
    contentFile << "  </office:body>\n";
    contentFile << "</office:document-content>\n";
    contentFile.close();
    
    // Create the ODT file using zip
    std::string zipCmd = "cd " + tempDir + " && zip -r " + _testDocPath + " mimetype META-INF/ content.xml > /dev/null 2>&1";
    std::cout << "[DEBUG] createTestDocument() - Running zip command: " << zipCmd << std::endl;
    int result = system(zipCmd.c_str());
    std::cout << "[DEBUG] createTestDocument() - Zip command result: " << result << std::endl;
    
    // Clean up temp directory
    std::string cleanupCmd = "rm -rf " + tempDir;
    system(cleanupCmd.c_str());
    
    std::cout << "[DEBUG] createTestDocument() - Test document created successfully" << std::endl;
    std::cout << "[DEBUG] createTestDocument() - END" << std::endl;
}

void DesktopLOKTest::setUp()
{
    std::cout << "[DEBUG] setUp() - START" << std::endl;
    _loKit = nullptr;
    std::cout << "[DEBUG] setUp() - _loKit set to nullptr" << std::endl;
    _loKitDocument = nullptr;
    std::cout << "[DEBUG] setUp() - _loKitDocument set to nullptr" << std::endl;
    
    // Create test document
    createTestDocument();
    
    std::cout << "[DEBUG] setUp() - END" << std::endl;
}

void DesktopLOKTest::tearDown()
{
    std::cout << "[DEBUG] tearDown() - START" << std::endl;
    if (_loKitDocument)
    {
        std::cout << "[DEBUG] tearDown() - _loKitDocument is not null, deleting..." << std::endl;
        delete _loKitDocument;
        _loKitDocument = nullptr;
        std::cout << "[DEBUG] tearDown() - _loKitDocument deleted and set to nullptr" << std::endl;
    }
    else
    {
        std::cout << "[DEBUG] tearDown() - _loKitDocument is null, skipping delete" << std::endl;
    }
    
    if (_loKit)
    {
        std::cout << "[DEBUG] tearDown() - Deleting _loKit..." << std::endl;
        delete _loKit;
        _loKit = nullptr;
        std::cout << "[DEBUG] tearDown() - _loKit deleted and set to nullptr" << std::endl;
    }
    
    // Clean up test document
    if (!_testDocPath.empty())
    {
        std::cout << "[DEBUG] tearDown() - Removing test document: " << _testDocPath << std::endl;
        unlink(_testDocPath.c_str());
    }
    
    std::cout << "[DEBUG] tearDown() - END" << std::endl;
}

void DesktopLOKTest::testSetView()
{
    std::cout << "[DEBUG] testSetView() - START" << std::endl;
    
    // Sequence: lok::Document::setView,lok::Document::registerCallback,lok::Office::registerCallback,lok::Document::destroyView,lok::Document::getViewsCount

    const char* loPath = std::getenv("LO_PATH");
    std::cout << "[DEBUG] testSetView() - Retrieved LO_PATH from environment" << std::endl;
    if (!loPath)
    {
        loPath = "/usr/lib/libreoffice/program";
        std::cout << "[DEBUG] testSetView() - LO_PATH not set, using default: " << loPath << std::endl;
    }
    else
    {
        std::cout << "[DEBUG] testSetView() - LO_PATH set to: " << loPath << std::endl;
    }

    std::cout << "[DEBUG] testSetView() - Calling lok::lok_cpp_init() with path: " << loPath << std::endl;
    _loKit = lok::lok_cpp_init(loPath);
    std::cout << "[DEBUG] testSetView() - lok::lok_cpp_init() returned: " << (void*)_loKit << std::endl;
    CPPUNIT_ASSERT_MESSAGE("Failed to initialize LibreOfficeKit", _loKit != nullptr);
    std::cout << "[DEBUG] testSetView() - LibreOfficeKit initialized successfully" << std::endl;

    const char* testDocPath = _testDocPath.c_str();
    std::cout << "[DEBUG] testSetView() - Using test document path: " << testDocPath << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKit->documentLoad() with path: " << testDocPath << std::endl;
    _loKitDocument = _loKit->documentLoad(testDocPath);
    std::cout << "[DEBUG] testSetView() - _loKit->documentLoad() returned: " << (void*)_loKitDocument << std::endl;
    CPPUNIT_ASSERT_MESSAGE("Failed to load document via LibreOfficeKit", _loKitDocument != nullptr);
    std::cout << "[DEBUG] testSetView() - Document loaded successfully" << std::endl;

    // Get initial view count
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getViewsCount()" << std::endl;
    int viewCount = _loKitDocument->getViewsCount();
    std::cout << "[DEBUG] testSetView() - Initial view count: " << viewCount << std::endl;
    CPPUNIT_ASSERT_MESSAGE("No views available, cannot proceed", viewCount > 0);
    std::cout << "[DEBUG] testSetView() - View count assertion passed (viewCount > 0)" << std::endl;

    // Get the current view ID
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getView()" << std::endl;
    int currentViewId = _loKitDocument->getView();
    std::cout << "[DEBUG] testSetView() - Current view ID: " << currentViewId << std::endl;

    // Create an additional view to test setView properly
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->createView()" << std::endl;
    int newViewId = _loKitDocument->createView();
    std::cout << "[DEBUG] testSetView() - New view ID created: " << newViewId << std::endl;
    CPPUNIT_ASSERT_MESSAGE("Failed to create new view", newViewId >= 0);
    std::cout << "[DEBUG] testSetView() - New view creation assertion passed (newViewId >= 0)" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getViewsCount() after creating new view" << std::endl;
    viewCount = _loKitDocument->getViewsCount();
    std::cout << "[DEBUG] testSetView() - View count after creating new view: " << viewCount << std::endl;

    // Exercise the API sequence - switch to the original view
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->setView(" << currentViewId << ")" << std::endl;
    _loKitDocument->setView(currentViewId);
    std::cout << "[DEBUG] testSetView() - setView() called with currentViewId: " << currentViewId << std::endl;
    
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getView() to verify active view" << std::endl;
    int activeView = _loKitDocument->getView();
    std::cout << "[DEBUG] testSetView() - Active view ID after setView: " << activeView << " (expected: " << currentViewId << ")" << std::endl;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("setView failed - view ID mismatch", currentViewId, activeView);
    std::cout << "[DEBUG] testSetView() - setView assertion passed for currentViewId" << std::endl;

    // Switch to the new view
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->setView(" << newViewId << ")" << std::endl;
    _loKitDocument->setView(newViewId);
    std::cout << "[DEBUG] testSetView() - setView() called with newViewId: " << newViewId << std::endl;
    
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getView() to verify active view" << std::endl;
    activeView = _loKitDocument->getView();
    std::cout << "[DEBUG] testSetView() - Active view ID after setView: " << activeView << " (expected: " << newViewId << ")" << std::endl;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("setView failed - view ID mismatch", newViewId, activeView);
    std::cout << "[DEBUG] testSetView() - setView assertion passed for newViewId" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->registerCallback(nullptr, nullptr)" << std::endl;
    _loKitDocument->registerCallback(nullptr, nullptr);
    std::cout << "[DEBUG] testSetView() - _loKitDocument->registerCallback() completed" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKit->registerCallback(nullptr, nullptr)" << std::endl;
    _loKit->registerCallback(nullptr, nullptr);
    std::cout << "[DEBUG] testSetView() - _loKit->registerCallback() completed" << std::endl;

    // Destroy the newly created view
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->destroyView(" << newViewId << ")" << std::endl;
    _loKitDocument->destroyView(newViewId);
    std::cout << "[DEBUG] testSetView() - destroyView() completed for view ID: " << newViewId << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getViewsCount() after destroying view" << std::endl;
    viewCount = _loKitDocument->getViewsCount();
    std::cout << "[DEBUG] testSetView() - Final view count: " << viewCount << std::endl;
    
    std::cout << "[DEBUG] testSetView() - END (SUCCESS)" << std::endl;
}

int main(int argc, char* argv[])
{
    std::cout << "[DEBUG] main() - START" << std::endl;
    std::cout << "[DEBUG] main() - argc: " << argc << std::endl;
    
    (void)argc;
    (void)argv;
    
    std::cout << "[DEBUG] main() - Creating CppUnit::TextUi::TestRunner" << std::endl;
    CppUnit::TextUi::TestRunner runner;
    
    std::cout << "[DEBUG] main() - Getting test factory registry" << std::endl;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
    
    std::cout << "[DEBUG] main() - Adding test to runner" << std::endl;
    runner.addTest(registry.makeTest());
    
    std::cout << "[DEBUG] main() - Running tests..." << std::endl;
    bool wasSuccessful = runner.run("", false);
    std::cout << "[DEBUG] main() - Test run completed. Result: " << (wasSuccessful ? "SUCCESS" : "FAILURE") << std::endl;
    
    std::cout << "[DEBUG] main() - END (returning " << (wasSuccessful ? 0 : 1) << ")" << std::endl;
    return wasSuccessful ? 0 : 1;
}
