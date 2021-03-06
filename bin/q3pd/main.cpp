/*
 * main.cpp
 * 
 * This is the Q3P-daemon main startup file.
 *
 * Author: Oliver Maurhart, <oliver.maurhart@ait.ac.at>
 *
 * Copyright (C) 2012-2016 AIT Austrian Institute of Technology
 * AIT Austrian Institute of Technology GmbH
 * Donau-City-Strasse 1 | 1220 Vienna | Austria
 * http://www.ait.ac.at
 *
 * This file is part of the AIT QKD Software Suite.
 *
 * The AIT QKD Software Suite is free software: you can redistribute 
 * it and/or modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 * 
 * The AIT QKD Software Suite is distributed in the hope that it will 
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with the AIT QKD Software Suite. 
 * If not, see <http://www.gnu.org/licenses/>.
 */

 
// ------------------------------------------------------------
// incs

#include <iostream>

#include <boost/program_options.hpp>

// ait
#include <qkd/utility/dbus.h>
#include <qkd/utility/syslog.h>
#include <qkd/version.h>

#include "node.h"


// ------------------------------------------------------------
// code


/**
 * start
 * 
 * @param   argc        as usual
 * @param   argv        as usual
 * @return  as usual
 */
int main(int argc, char ** argv) {
    
    std::string sApplication = std::string("q3pd - AIT Q3P Node V") + qkd::version();
    std::string sDescription = std::string("\nThis is a Q3P node daemon.\n\nCopyright 2012-2016 AIT Austrian Institute of Technology GmbH");
    std::string sSynopsis = std::string("Usage: ") + argv[0] + " [OPTIONS] ID";
    
    boost::program_options::options_description cOptions(sApplication + "\n" + sDescription + "\n\n\t" + sSynopsis + "\n\nAllowed Options");
    cOptions.add_options()("config,c", boost::program_options::value<std::string>(), "configuration file URL");
    cOptions.add_options()("debug,d", "enable debug output on stderr");
    cOptions.add_options()("help,h", "this page");
    cOptions.add_options()("version,v", "print version string");
    
    boost::program_options::options_description cArgs("Arguments");
    cArgs.add_options()("ID", "ID is the identifier or name of the node");
    boost::program_options::positional_options_description cPositionalDescription; 
    cPositionalDescription.add("ID", 1);
    
    boost::program_options::options_description cCmdLineOptions("Command Line");
    cCmdLineOptions.add(cOptions);
    cCmdLineOptions.add(cArgs);

    boost::program_options::variables_map cVariableMap;
    
    try {
        boost::program_options::command_line_parser cParser(argc, argv);
        boost::program_options::store(cParser.options(cCmdLineOptions).positional(cPositionalDescription).run(), cVariableMap);
        boost::program_options::notify(cVariableMap);        
    }
    catch (std::exception & cException) {
        std::cerr << "error parsing command line: " << cException.what() << "\ntype '--help' for help" << std::endl;        
        return 1;
    }
    
    if (cVariableMap.count("help")) {
        std::cout << cOptions << std::endl;
        std::cout << cArgs.find("ID", false).description() << "\n" << std::endl;      
        return 0;
    }
    
    if (cVariableMap.count("version")) {
        std::cout << sApplication << std::endl;
        return 0;
    }
    
    if (cVariableMap.count("debug")) qkd::utility::debug::enabled() = true;

    if (cVariableMap.count("ID") != 1) {
        std::cerr << "need exactly one ID argument" << "\ntype '--help' for help" << std::endl;
        return 1;
    }
    
    std::string sId = cVariableMap["ID"].as<std::string>();

    if (!qkd::utility::dbus::valid_service_name_particle(sId)) {
        qkd::utility::syslog::crit() << __FILENAME__ << '@' << __LINE__ << ": " << "the given ID ('" << sId << "') cannot be used as a DBus service name, please consider another name.";
        std::cerr << "the given ID ('" << sId << "') cannot be used as a DBus service name, please consider another name." << std::endl;
        return 1;
    }
    
    qkd::utility::debug() << "AIT Q3P Node " << QKD_VERSION << " Node-ID: " << sId;
    
    QCoreApplication cCoreApplication(argc, argv);
    cCoreApplication.setOrganizationName("AIT Austrian Institute of Technology GmbH");
    cCoreApplication.setOrganizationDomain("ait.ac.at");
    cCoreApplication.setApplicationName("Q3P Daemon");
    cCoreApplication.setApplicationVersion(QKD_VERSION);
    
    std::string sConfigFileURL;
    if (cVariableMap.count("config")) sConfigFileURL = cVariableMap["config"].as<std::string>();
    
    qkd::q3p::node cNode(QString::fromStdString(sId), QString::fromStdString(sConfigFileURL));

    cCoreApplication.exec();
    
    return 0;
}

