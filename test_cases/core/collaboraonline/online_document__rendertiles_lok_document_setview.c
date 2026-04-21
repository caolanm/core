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
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

// Sequence: lok::Document::setView,lok::Document::registerCallback,lok::Office::registerCallback,lok::Document::destroyView,lok::Document::getViewsCount

static void documentCallback(int type, const char* payload, void* data) {
    // Document callback handler
    std::cout << "[DEBUG] documentCallback called: type=" << type << ", payload=" << (payload ? payload : "NULL") << std::endl;
    (void)type;
    (void)payload;
    (void)data;
}

static void officeCallback(int type, const char* payload, void* data) {
    // Office callback handler
    std::cout << "[DEBUG] officeCallback called: type=" << type << ", payload=" << (payload ? payload : "NULL") << std::endl;
    (void)type;
    (void)payload;
    (void)data;
}

// Helper function to create a minimal ODT file
static void createTestODTFile(const char* filename) {
    std::cout << "[DEBUG] createTestODTFile() - Creating test ODT file: " << filename << std::endl;
    
    // Create a minimal ODT file structure
    // ODT files are ZIP archives, but for testing we'll create a simple text file
    // and let LibreOffice try to open it, or create a proper minimal ODT
    
    // For simplicity, we'll create a minimal flat XML ODT file
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to create test file: " << filename << std::endl;
        return;
    }
    
    // Write a minimal flat ODT XML content
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<office:document xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" ";
    file << "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" ";
    file << "xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" ";
    file << "xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" ";
    file << "office:mimetype=\"application/vnd.oasis.opendocument.text\">\n";
    file << "  <office:body>\n";
    file << "    <office:text>\n";
    file << "      <text:p>Test Document</text:p>\n";
    file << "    </office:text>\n";
    file << "  </office:body>\n";
    file << "</office:document>\n";
    
    file.close();
    std::cout << "[DEBUG] createTestODTFile() - Test file created successfully" << std::endl;
}

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
};

CPPUNIT_TEST_SUITE_REGISTRATION(DesktopLOKTest);

void DesktopLOKTest::setUp()
{
    std::cout << "[DEBUG] setUp() - START" << std::endl;
    _loKit = nullptr;
    std::cout << "[DEBUG] setUp() - _loKit set to nullptr" << std::endl;
    _loKitDocument = nullptr;
    std::cout << "[DEBUG] setUp() - _loKitDocument set to nullptr" << std::endl;
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
    std::cout << "[DEBUG] tearDown() - deleting _loKit..." << std::endl;
    delete _loKit;
    _loKit = nullptr;
    std::cout << "[DEBUG] tearDown() - _loKit deleted and set to nullptr" << std::endl;
    std::cout << "[DEBUG] tearDown() - END" << std::endl;
}

void DesktopLOKTest::testSetView()
{
    std::cout << "[DEBUG] testSetView() - START" << std::endl;
    
    const char* loPath = std::getenv("LO_PATH");
    std::cout << "[DEBUG] testSetView() - Retrieved LO_PATH from environment: " << (loPath ? loPath : "NULL") << std::endl;
    if (!loPath)
    {
        loPath = "/usr/lib/libreoffice/program";
        std::cout << "[DEBUG] testSetView() - LO_PATH was null, using default: " << loPath << std::endl;
    }

    std::cout << "[DEBUG] testSetView() - Calling lok::lok_cpp_init with path: " << loPath << std::endl;
    _loKit = lok::lok_cpp_init(loPath);
    std::cout << "[DEBUG] testSetView() - lok::lok_cpp_init returned: " << (void*)_loKit << std::endl;
    CPPUNIT_ASSERT_MESSAGE("Failed to initialize LibreOfficeKit", _loKit != nullptr);
    std::cout << "[DEBUG] testSetView() - LibreOfficeKit initialized successfully" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKit->registerCallback(officeCallback, nullptr)" << std::endl;
    _loKit->registerCallback(officeCallback, nullptr);
    std::cout << "[DEBUG] testSetView() - Office callback registered" << std::endl;

    const char* testDocPath = std::getenv("TEST_DOC_PATH");
    std::cout << "[DEBUG] testSetView() - Retrieved TEST_DOC_PATH from environment: " << (testDocPath ? testDocPath : "NULL") << std::endl;
    
    // Create a test document if not provided
    char tempDocPath[256];
    if (!testDocPath)
    {
        // Get current working directory
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            std::cout << "[DEBUG] testSetView() - Current working directory: " << cwd << std::endl;
        }
        
        snprintf(tempDocPath, sizeof(tempDocPath), "/tmp/test_document_%d.fodt", getpid());
        testDocPath = tempDocPath;
        std::cout << "[DEBUG] testSetView() - TEST_DOC_PATH was null, creating test file: " << testDocPath << std::endl;
        createTestODTFile(testDocPath);
        
        // Verify file exists
        struct stat buffer;
        if (stat(testDocPath, &buffer) == 0) {
            std::cout << "[DEBUG] testSetView() - Test file exists, size: " << buffer.st_size << " bytes" << std::endl;
        } else {
            std::cout << "[ERROR] testSetView() - Test file does not exist after creation!" << std::endl;
        }
    }

    std::cout << "[DEBUG] testSetView() - Calling _loKit->documentLoad with path: " << testDocPath << std::endl;
    _loKitDocument = _loKit->documentLoad(testDocPath);
    std::cout << "[DEBUG] testSetView() - _loKit->documentLoad returned: " << (void*)_loKitDocument << std::endl;
    
    if (_loKitDocument == nullptr) {
        std::cout << "[ERROR] testSetView() - Failed to load document. Checking for error..." << std::endl;
        const char* error = _loKit->getError();
        if (error) {
            std::cout << "[ERROR] testSetView() - LibreOfficeKit error: " << error << std::endl;
        }
    }
    
    CPPUNIT_ASSERT_MESSAGE("Failed to load document via LibreOfficeKit", _loKitDocument != nullptr);
    std::cout << "[DEBUG] testSetView() - Document loaded successfully" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->registerCallback(documentCallback, nullptr)" << std::endl;
    _loKitDocument->registerCallback(documentCallback, nullptr);
    std::cout << "[DEBUG] testSetView() - Document callback registered" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getViewsCount()" << std::endl;
    int initialViewsCount = _loKitDocument->getViewsCount();
    std::cout << "[DEBUG] testSetView() - Initial views count: " << initialViewsCount << std::endl;
    CPPUNIT_ASSERT_MESSAGE("Invalid initial views count", initialViewsCount > 0);
    std::cout << "[DEBUG] testSetView() - Initial views count is valid (> 0)" << std::endl;

    // Create an additional view to test setView properly
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->createView()" << std::endl;
    int newViewId = _loKitDocument->createView();
    std::cout << "[DEBUG] testSetView() - _loKitDocument->createView() returned: " << newViewId << std::endl;
    CPPUNIT_ASSERT_MESSAGE("Failed to create new view", newViewId >= 0);
    std::cout << "[DEBUG] testSetView() - New view created successfully with ID: " << newViewId << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getViewsCount() after create" << std::endl;
    int viewsCountAfterCreate = _loKitDocument->getViewsCount();
    std::cout << "[DEBUG] testSetView() - Views count after create: " << viewsCountAfterCreate << std::endl;
    std::cout << "[DEBUG] testSetView() - Expected: " << (initialViewsCount + 1) << ", Actual: " << viewsCountAfterCreate << std::endl;
    CPPUNIT_ASSERT_EQUAL(initialViewsCount + 1, viewsCountAfterCreate);
    std::cout << "[DEBUG] testSetView() - Views count assertion passed" << std::endl;

    // Test setView with the original view (0)
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->setView(0)" << std::endl;
    _loKitDocument->setView(0);
    std::cout << "[DEBUG] testSetView() - _loKitDocument->setView(0) completed" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getView() after setView(0)" << std::endl;
    int currentView = _loKitDocument->getView();
    std::cout << "[DEBUG] testSetView() - Current view after setView(0): " << currentView << std::endl;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("setView(0) failed", 0, currentView);
    std::cout << "[DEBUG] testSetView() - setView(0) assertion passed" << std::endl;

    // Test setView with the new view
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->setView(" << newViewId << ")" << std::endl;
    _loKitDocument->setView(newViewId);
    std::cout << "[DEBUG] testSetView() - _loKitDocument->setView(" << newViewId << ") completed" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getView() after setView(newViewId)" << std::endl;
    currentView = _loKitDocument->getView();
    std::cout << "[DEBUG] testSetView() - Current view after setView(newViewId): " << currentView << std::endl;
    std::cout << "[DEBUG] testSetView() - Expected: " << newViewId << ", Actual: " << currentView << std::endl;
    CPPUNIT_ASSERT_EQUAL_MESSAGE("setView(newViewId) failed", newViewId, currentView);
    std::cout << "[DEBUG] testSetView() - setView(newViewId) assertion passed" << std::endl;

    // Destroy the additional view
    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->destroyView(" << newViewId << ")" << std::endl;
    _loKitDocument->destroyView(newViewId);
    std::cout << "[DEBUG] testSetView() - _loKitDocument->destroyView(" << newViewId << ") completed" << std::endl;

    std::cout << "[DEBUG] testSetView() - Calling _loKitDocument->getViewsCount() after destroy" << std::endl;
    int finalViewsCount = _loKitDocument->getViewsCount();
    std::cout << "[DEBUG] testSetView() - Final views count: " << finalViewsCount << std::endl;
    std::cout << "[DEBUG] testSetView() - Expected: " << initialViewsCount << ", Actual: " << finalViewsCount << std::endl;
    CPPUNIT_ASSERT_EQUAL(initialViewsCount, finalViewsCount);
    std::cout << "[DEBUG] testSetView() - Final views count assertion passed" << std::endl;
    
    // Clean up temporary file if we created it
    if (testDocPath == tempDocPath) {
        std::cout << "[DEBUG] testSetView() - Removing temporary test file: " << testDocPath << std::endl;
        unlink(testDocPath);
    }
    
    std::cout << "[DEBUG] testSetView() - END (SUCCESS)" << std::endl;
}

int main(int argc, char* argv[])
{
    std::cout << "[DEBUG] main() - START" << std::endl;
    std::cout << "[DEBUG] main() - argc: " << argc << std::endl;
    for (int i = 0; i < argc; i++)
    {
        std::cout << "[DEBUG] main() - argv[" << i << "]: " << argv[i] << std::endl;
    }
    
    (void)argc;
    (void)argv;
    
    std::cout << "[DEBUG] main() - Creating CppUnit::TextUi::TestRunner" << std::endl;
    CppUnit::TextUi::TestRunner runner;
    std::cout << "[DEBUG] main() - Getting test registry" << std::endl;
    CppUnit::TestFactoryRegistry &registry = CppUnit::TestFactoryRegistry::getRegistry();
    std::cout << "[DEBUG] main() - Adding tests to runner" << std::endl;
    runner.addTest(registry.makeTest());
    std::cout << "[DEBUG] main() - Running tests..." << std::endl;
    bool wasSuccessful = runner.run("", false);
    std::cout << "[DEBUG] main() - Tests completed. wasSuccessful: " << (wasSuccessful ? "true" : "false") << std::endl;
    std::cout << "[DEBUG] main() - END (returning " << (wasSuccessful ? 0 : 1) << ")" << std::endl;
    return wasSuccessful ? 0 : 1;
}
