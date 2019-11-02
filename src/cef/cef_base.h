#ifndef DOOGIE_CEF_BASE_H_
#define DOOGIE_CEF_BASE_H_

// Ignore all of the unused parameters
#if defined(_MSC_VER)
# pragma warning(push)
# pragma warning(disable: 4100)
#endif
#if defined(__GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_file_util.h"
#include "include/cef_parser.h"
#include "include/cef_request_context_handler.h"
#include "include/cef_urlrequest.h"
#include "include/cef_xml_reader.h"
#include "include/cef_zip_reader.h"
#include "include/wrapper/cef_xml_object.h"
#include "include/wrapper/cef_zip_archive.h"

// Restore unused parameter checks
#if defined(_MSC_VER)
# pragma warning(pop)
#endif
#if defined(__GNUC__)
# pragma GCC diagnostic pop
#endif

#endif  // DOOGIE_CEF_BASE_H_
