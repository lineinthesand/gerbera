/*  config_manager.cc - this file is part of MediaTomb.
                                                                                
    Copyright (C) 2005 Gena Batyan <bgeradz@deadlock.dhs.org>,
                       Sergey Bostandzhyan <jin@deadlock.dhs.org>
                                                                                
    MediaTomb is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
                                                                                
    MediaTomb is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
                                                                                
    You should have received a copy of the GNU General Public License
    along with MediaTomb; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include "uuid/uuid.h"
#include "common.h"
#include "config_manager.h"
#include "storage.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <server.h>

using namespace zmm;
using namespace mxml;

static Ref<ConfigManager> instance;

/*
static void check_path_exs(String path, bool needDir = true)
{
    int ret = 0;
    struct stat statbuf;

    ret = stat(path.c_str(), &statbuf);
    if (ret != 0)
        throw Exception(path + " : " + strerror(errno));

    if (needDir && (!S_ISDIR(statbuf.st_mode)))
        throw Exception(String("Not a directory: ") + path);
}

*/
String ConfigManager::constructPath(String path)
{
    if (path.charAt(0) == '/')
        return path;
    if (home == "." && path.charAt(0) == '.')
        return path;
    
    if (home == "")
        return String(".") + DIR_SEPARATOR + path;
    else
        return home + DIR_SEPARATOR + path;
}

ConfigManager::ConfigManager() : Object()
{
}

void ConfigManager::init(String filename, String userhome)
{
    instance = Ref<ConfigManager>(new ConfigManager());

    if (filename == nil)
    {
        try 
        {
            // we are looking for ~/.mediatomb
            check_path_ex(userhome + DIR_SEPARATOR + DEFAULT_CONFIG_HOME, true);
        }
        catch (Exception e)
        {
            throw Exception(String("\nThe server has not yet been set up. \n") +
                                   "Please run the tomb-install script or specify an alternative configuration\n" +
                                   "file on the command line. For a list of options use: mediatomb -h\n");
        }

        try
        {
            // we are looking for ~/.mediatomb/config.xml
            check_path_ex(userhome + DIR_SEPARATOR + DEFAULT_CONFIG_HOME + DIR_SEPARATOR + DEFAULT_CONFIG_NAME);
        }
        catch (Exception e)
        {
            throw Exception(String("\nThe server configuration file is missing. \n") +
                                   "Please run the tomb-install script or specify an alternative configuration\n" +
                                   "file on the command line. For a list of options use: mediatomb -h\n");
        }

        instance->load(userhome + DIR_SEPARATOR + DEFAULT_CONFIG_HOME + DIR_SEPARATOR + DEFAULT_CONFIG_NAME);
    }
    else
    {
        instance->load(filename);
    }

    instance->validate();
    instance->prepare();    
}

void ConfigManager::create()
{
}

void ConfigManager::validate()
{
    String temp;

    printf("Checking configuration...\n");
    
    if (root->getName() != "config")
        throw Exception("Error in config file: <config> tag not found");

    if (root->getChild("server") == nil)
        throw Exception("Error in config file: <server> tag not found");

    // validating storage settings and initializing storage
    try 
    {
        getOption("/server/storage/attribute::driver");
    }
    catch (Exception e)
    {
        throw Exception("Error in config file: missing \"driver\" attribute in <storage> element.");
    }

    try
    {
        temp = getOption("/server/storage/database-file");
    }
    catch (Exception e)
    {
        throw Exception("Error in config file: <database-file> tag not found in <storage> section.");
    }

    Storage::getInstance(PRIMARY_STORAGE);
    Storage::getInstance(FILESYSTEM_STORAGE);

    printf("Configuration check succeeded.\n");
}

void ConfigManager::prepare()
{
    bool need_to_save = false;
    String temp;

    home = getOption("/server/home");
  
    // first check the UDN, if it is empty we will create a nice one
    // and save it to configuration
    Ref<Element> element = root->getChild("server")->getChild("udn");
    if (element->getText() == nil || element->getText() == "")
    {
        char   uuid_str[37];
        uuid_t uuid;
                
        uuid_generate(uuid);
        uuid_unparse(uuid, uuid_str);

        printf("UUID GENERATED!: %s\n", uuid_str);
        
        element->setText(String("uuid:") + uuid_str);

        need_to_save = true;
    }
    
    if (need_to_save)
        save();

    // well.. for now we will initialize stuff that does not belong to 
    // one specific module here... 
    fs_charset = getOption("/import/filesystem-charset", DEFAULT_FILESYSTEM_CHARSET);
    printf("Using filesystem charset: %s\n", fs_charset.c_str());

    getOption("/server/name", DESC_FRIENDLY_NAME);

    temp = getOption("/server/ui/attribute::enabled", DEFAULT_UI_VALUE);
    if ((!string_ok(temp)) || ((temp != "yes") && (temp != "no")))
        throw Exception("Error in config file: incorrect parameter for <ui enabled=\"\" /> attribute");

    // now go through the modules and let them check their options and set
    // default values
    Ref<Server> s(new Server());
    s->configure();
}

Ref<ConfigManager> ConfigManager::getInstance()
{
    return instance;
}


void ConfigManager::save()
{
    save(filename);
}

void ConfigManager::save(String filename)
{
    String content = root->print();

    FILE *file = fopen(filename.c_str(), "wb");
    if (file == NULL)
    {
        throw Exception(String("could not open config file ") +
                        filename + " for writing : " + strerror(errno));
    }

    int bytesWritten = fwrite(content.c_str(), sizeof(char),
                              content.length(), file);
    if (bytesWritten < content.length())
    {
        throw Exception(String("could not write to config file ") +
                        filename + " : " + strerror(errno));
    }
    
    fclose(file);
}

void ConfigManager::load(String filename)
{
    this->filename = filename;
    Ref<Parser> parser(new Parser());
    root = parser->parseFile(filename);
}

String ConfigManager::getOption(String xpath, String def)
{      
    Ref<XPath> rootXPath(new XPath(root));
    String value = rootXPath->getText(xpath);
    if (value != nil)
        return value;

    printf("Config: option not found: %s using default value: %s\n",
           xpath.c_str(), def.c_str());
    
    String pathPart = XPath::getPathPart(xpath);
    String axisPart = XPath::getAxisPart(xpath);

    Ref<Array<StringBase> >parts = split_string(pathPart, '/');
    
    Ref<Element> cur = root;
    String attr = nil;
    
    int i;
    Ref<Element> child;
    for (i = 0; i < parts->size(); i++)
    {
        String part = parts->get(i);
        child = cur->getChild(part);
        if (child == nil)
            break;
        cur = child;
    }
    // here cur is the last existing element in the path
    for (; i < parts->size(); i++)
    {
        String part = parts->get(i);
        child = Ref<Element>(new Element(part));
        cur->appendChild(child);
        cur = child;
    }
    
    if (axisPart != nil)
    {
        String axis = XPath::getAxis(axisPart);
        String spec = XPath::getSpec(axisPart);
        if (axis != "attribute")
        {
            throw Exception("ConfigManager::getOption: only attribute:: axis supported");
        }
        cur->setAttribute(spec, def);
    }

    cur->setText(def);

    return def;
}

int ConfigManager::getIntOption(String xpath, int def)
{
    String sDef;

    sDef = String::from(def);
    
    String sVal = getOption(xpath, sDef);
    return sVal.toInt();
}

String ConfigManager::getOption(String xpath)
{      
    Ref<XPath> rootXPath(new XPath(root));
    String value = rootXPath->getText(xpath);
    if (value != nil)
        return value;
    throw Exception(String("Config: option not found: ") + xpath);
}

int ConfigManager::getIntOption(String xpath)
{
    String sVal = getOption(xpath);
    int val = sVal.toInt();
    return val;
}


Ref<Element> ConfigManager::getElement(String xpath)
{      
    Ref<XPath> rootXPath(new XPath(root));
    return rootXPath->getElement(xpath);
}

void ConfigManager::writeBookmark(String ip, String port)
{
    FILE    *f;
    String  filename;
    String  path;
    String  data; 
    int     size; 
  
    String value = ConfigManager::getInstance()->getOption("/server/ui/attribute::enabled");
    if (value != "yes")
    {
        data = http_redirect_to(ip, port, "disabled.html").c_str();
    }
    else
    {
        data = http_redirect_to(ip, port).c_str();
    }

    filename = getOption("/server/bookmark", "mediatomb.html");
    path = constructPath(filename);
    
        
    f = fopen(path.c_str(), "w");
    if (f == NULL)
    {
        throw Exception(String("writeBookmark: failed to open: ") + path.c_str());
    }

    size = fwrite(data.c_str(), sizeof(char), data.length(), f);
    fclose(f);

    if (size < data.length())
        throw Exception(String("write_Bookmark: failed to write to: ") + path.c_str());

}

String ConfigManager::checkOptionString(String xpath)
{
    String temp = getOption(xpath);
    if (!string_ok(temp))
        throw Exception(String("Config: value of ") + xpath + " tag is invalid");

    return temp;
}

