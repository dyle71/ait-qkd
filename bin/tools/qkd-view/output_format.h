/*
 * output_format.h
 *
 * This is shows the current QKD system snapshot
 *
 * Author: Manuel Warum, <manuel.warum@ait.ac.at>
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

#ifndef QKD_OUTPUT_FORMAT_H
#define QKD_OUTPUT_FORMAT_H

#include <ostream>
#include <memory>
#include <qkd/utility/investigation.h>


class output_format {
public:
    struct configuration_options {
        bool bOnlyModuleIO = false;
        bool bOmitHeader = false;
        bool bOutputShort = false;
        bool bOutputAsJSON = false;
    };

    virtual void initialize(configuration_options const &cProgramOptions) = 0;
    virtual void write(std::ostream &cOut, qkd::utility::investigation & cInvestigation) = 0;

    static std::shared_ptr<output_format> create(configuration_options const & cOptions);
};


#endif //QKD_OUTPUT_FORMAT_H
