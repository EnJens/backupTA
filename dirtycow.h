/*
 *   This file is part of BackupTA.

 *  BackupTA is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  BackupTA is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with BackupTA.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _DIRTYCOW_H_
#define _DIRTYCOW_H_

int overwrite_file(const char *target, const char *source, size_t length);

#endif /* _DIRTYCOW_H_ */
