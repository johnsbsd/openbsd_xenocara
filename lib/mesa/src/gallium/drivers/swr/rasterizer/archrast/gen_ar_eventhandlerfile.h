/****************************************************************************
* Copyright (C) 2016 Intel Corporation.   All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*
* @file gen_ar_eventhandlerfile.h
*
* @brief Event handler interface.  auto-generated file
* 
* DO NOT EDIT
* 
******************************************************************************/
#pragma once

#include "common/os.h"
#include "gen_ar_eventhandler.h"
#include <fstream>
#include <sstream>

namespace ArchRast
{
    //////////////////////////////////////////////////////////////////////////
    /// EventHandlerFile - interface for handling events.
    //////////////////////////////////////////////////////////////////////////
    class EventHandlerFile : public EventHandler
    {
    public:
        EventHandlerFile(uint32_t id)
        {
#if defined(_WIN32)
            DWORD pid = GetCurrentProcessId();
            TCHAR procname[MAX_PATH];
            GetModuleFileName(NULL, procname, MAX_PATH);
            const char* pBaseName = strrchr(procname, '\\');
            std::stringstream outDir;
            outDir << KNOB_DEBUG_OUTPUT_DIR << pBaseName << "_" << pid << std::ends;
            CreateDirectory(outDir.str().c_str(), NULL);

            char buf[255];
            // There could be multiple threads creating thread pools. We
            // want to make sure they are uniquly identified by adding in
            // the creator's thread id into the filename.
            sprintf(buf, "%s\\ar_event%d_%d.bin", outDir.str().c_str(), GetCurrentThreadId(), id);
            mFilename = std::string(buf);
#else
            SWR_ASSERT(0);
#endif
        }

        ~EventHandlerFile()
        {
            if (mFile.is_open()) mFile.close();
        }

        void write(uint32_t eventId, const char* pBlock, uint32_t size)
        {
            if (!mFile.is_open())
            {
                mFile.open(mFilename, std::ios::out | std::ios::app | std::ios::binary);
            }

            mFile.write((char*)&eventId, sizeof(eventId));
            mFile.write(pBlock, size);
        }

        virtual void handle(Start& event)
        {
            write(1, (char*)&event.data, sizeof(event.data));
        }
        virtual void handle(End& event)
        {
            write(2, (char*)&event.data, sizeof(event.data));
        }
        virtual void handle(DrawInstancedEvent& event)
        {
            write(3, (char*)&event.data, sizeof(event.data));
        }
        virtual void handle(DrawIndexedInstancedEvent& event)
        {
            write(4, (char*)&event.data, sizeof(event.data));
        }
        virtual void handle(DispatchEvent& event)
        {
            write(5, (char*)&event.data, sizeof(event.data));
        }
        virtual void handle(FrameEndEvent& event)
        {
            write(6, (char*)&event.data, sizeof(event.data));
        }
        virtual void handle(FrontendStatsEvent& event)
        {
            write(7, (char*)&event.data, sizeof(event.data));
        }
        virtual void handle(BackendStatsEvent& event)
        {
            write(8, (char*)&event.data, sizeof(event.data));
        }

        std::ofstream mFile;
        std::string mFilename;
    };
}
