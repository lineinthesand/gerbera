/*MT*
    
    MediaTomb - http://www.mediatomb.cc/
    
    script.h - this file is part of MediaTomb.
    
    Copyright (C) 2005 Gena Batyan <bgeradz@mediatomb.cc>,
                       Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>
    
    Copyright (C) 2006-2007 Gena Batyan <bgeradz@mediatomb.cc>,
                            Sergey 'Jin' Bostandzhyan <jin@mediatomb.cc>,
                            Leonhard Wimmer <leo@mediatomb.cc>
    
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.
    
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    version 2 along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
    
    $Id$
*/

/// \file script.h

#ifndef __SCRIPTING_SCRIPT_H__
#define __SCRIPTING_SCRIPT_H__

#ifdef __APPLE__
    #define XP_MAC 1
#else
    #define XP_UNIX 1
#endif

#include <jsapi.h>
#include "common.h"
#include "runtime.h"
#include "cds_objects.h"

typedef enum
{
    S_IMPORT = 0,
    S_PLAYLIST
} script_class_t;

class Script : public zmm::Object
{
public:
    zmm::Ref<Runtime> runtime;
    JSRuntime *rt;
    JSContext *cx;
    JSObject  *glob;
    JSScript *script;
    JSScript *common_script;
    
public:
    Script(zmm::Ref<Runtime> runtime);
    virtual ~Script();
    
    zmm::String getProperty(JSObject *obj, zmm::String name);
    int getBoolProperty(JSObject *obj, zmm::String name);
    int getIntProperty(JSObject *obj, zmm::String name, int def);
    JSObject *getObjectProperty(JSObject *obj, zmm::String name);
    
    void setProperty(JSObject *obj, zmm::String name, zmm::String value);
    void setIntProperty(JSObject *obj, zmm::String name, int value);
    void setObjectProperty(JSObject *parent, zmm::String name, JSObject *obj);
    
    void deleteProperty(JSObject *obj, zmm::String name);
    
    JSObject *getGlobalObject();
    void setGlobalObject(JSObject *glob);
    
    JSContext *getContext();
    
    void defineFunction(zmm::String name, JSNative function, int numParams);
    void defineFunctions(JSFunctionSpec *functions);
    void load(zmm::String scriptPath);
    void load(zmm::String scriptText, zmm::String scriptPath);
    
    zmm::Ref<CdsObject> jsObject2cdsObject(JSObject *js, zmm::Ref<CdsObject> pcd);
    void cdsObject2jsObject(zmm::Ref<CdsObject> obj, JSObject *js);
    
    virtual script_class_t whoami() = 0;
    
protected:
    void execute();
    
private:
    void initGlobalObject();
    JSScript *_load(zmm::String scriptPath);
    void _execute(JSScript *scr);
};

#endif // __SCRIPTING_SCRIPT_H__