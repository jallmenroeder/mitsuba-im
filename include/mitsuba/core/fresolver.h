/*
    This file is part of Mitsuba, a physically based rendering system.

    Copyright (c) 2007-2014 by Wenzel Jakob and others.

    Mitsuba is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License Version 3
    as published by the Free Software Foundation.

    Mitsuba is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#if !defined(__MITSUBA_CORE_FRESOLVER_H_)
#define __MITSUBA_CORE_FRESOLVER_H_

#include <mitsuba/mitsuba.h>

MTS_NAMESPACE_BEGIN

/**
 * \brief File resolution helper
 *
 * FileResolver is a convenience class that allows searching for files
 * within a set of specifiable search paths in a cross-platform
 * compatible manner (similar to the $PATH variable on various
 * operating systems).
 *
 * \ingroup libcore
 * \ingroup libpython
 */
class MTS_EXPORT_CORE FileResolver : public Object {
public:
	/**
	 * \brief Create a new file resolver with the default settings
	 *
	 * Create a new file resolver containing the current working
	 * directory as the initial search path.
	 */
	FileResolver();
	FileResolver(const FileResolver&);
	FileResolver& operator =(const FileResolver&) = delete;

	/**
	 * \brief Resolve a file using the stored list of search paths
	 *
	 * Go through the list of search paths and try to resolve the
	 * supplied path with respect to each one. If everything fails,
	 * the path is returned as-is.
	 */
	fs::pathstr resolve(fs::pathstr const& path) const;

	/**
	 * \brief Resolve a file using the stored list of search paths
	 *
	 * In comparison to \ref resolve(), this funtion returns all
	 * matches instead of only the first one.
	 */
	std::vector<fs::pathstr> resolveAll(fs::pathstr const& path) const;

	/**
	 * \brief Does the same as \ref resolve(), but returns an
	 * absolute path.
	 */
	fs::pathstr resolveAbsolute(fs::pathstr const& path) const;

	/// Create a clone of the file resolver
	FileResolver *clone() const;

	/// Append a search path to the resolver
	void appendPath(fs::pathstr const& path);

	/// Prepend a search path to the resolver
	void prependPath(fs::pathstr const& path);

	/// Clear all stored search paths
	void clear();

	/// Return the number of stored paths
	size_t getPathCount() const;

	/// Return one of the stored paths
	fs::pathstr getPath(size_t index) const;

	/// Return a human-readable string representation
	std::string toString() const;

	MTS_DECLARE_CLASS()
protected:
	virtual ~FileResolver();
private:
	std::vector<fs::pathdat> m_paths;
};

MTS_NAMESPACE_END

#endif /* __MITSUBA_CORE_FRESOLVER_H_ */
