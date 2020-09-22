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

#include <mitsuba/core/properties.h>
#include <mitsuba/core/netobject.h>
#include <mitsuba/core/track.h>

/* Keep the boost::variant includes outside of properties.h,
   since they noticeably add to the overall compile times */
#include <variant>

MTS_NAMESPACE_BEGIN

typedef std::variant<
	bool, int64_t, Float, Point, Vector, Transform, AnimatedTransform *,
	Spectrum, std::string, Properties::Data> ElementData;

struct PropertyElement {
	ElementData data;
	short lastWrite;
	mutable bool queried;
};

#define MUTABLE_RECORD(setter, default) do { \
		if (m_mutateToRecord) { \
			const_cast<Properties*>(this)->setter(name, default); \
		}  \
		return default; \
	} while (false)

#define DEFINE_PROPERTY_ACCESSOR(Type, BaseType, TypeName, ReadableName) \
	void Properties::set##TypeName(const std::string &name, const Type &value, bool warnDuplicates) { \
		if (hasProperty(name) && warnDuplicates) \
			SLog(EWarn, "Property \"%s\" was specified multiple times!", name.c_str()); \
		(*m_elements)[name].data = (BaseType) value; \
		(*m_elements)[name].lastWrite = m_setCounter++; \
		(*m_elements)[name].queried = false; \
	} \
	\
	Type Properties::get##TypeName(const std::string &name) const { \
		std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name); \
		if (it == m_elements->end()) { \
			SLog(m_mutateToRecord ? EWarn : EError, "Property \"%s\" has not been specified!", name.c_str()); \
			MUTABLE_RECORD(set##TypeName, Type()); \
		} \
		const BaseType *result = std::get_if<BaseType>(&it->second.data); \
		if (!result) \
			SLog(EError, "The property \"%s\" has the wrong type (expected <" #ReadableName ">). The " \
					"complete property record is :\n%s", name.c_str(), toString().c_str()); \
		it->second.queried = true; \
		return (Type) *result; \
	} \
	\
	Type Properties::get##TypeName(const std::string &name, const Type &defVal) const { \
		std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name); \
		if (it == m_elements->end()) \
			MUTABLE_RECORD(set##TypeName, defVal); \
		const BaseType *result = std::get_if<BaseType>(&it->second.data); \
		if (!result) \
			SLog(EError, "The property \"%s\" has the wrong type (expected <" #ReadableName ">). The " \
					"complete property record is :\n%s", name.c_str(), toString().c_str()); \
		it->second.queried = true; \
		return (Type) *result; \
	}

DEFINE_PROPERTY_ACCESSOR(bool, bool, Boolean, boolean)
DEFINE_PROPERTY_ACCESSOR(int64_t, int64_t, Long, integer)
DEFINE_PROPERTY_ACCESSOR(int, int64_t, Integer, integer)
DEFINE_PROPERTY_ACCESSOR(size_t, int64_t, Size, integer)
DEFINE_PROPERTY_ACCESSOR(Float, Float, Float, float)
DEFINE_PROPERTY_ACCESSOR(Point, Point, Point, point)
DEFINE_PROPERTY_ACCESSOR(Vector, Vector, Vector, vector)
DEFINE_PROPERTY_ACCESSOR(Transform, Transform, Transform, transform)
DEFINE_PROPERTY_ACCESSOR(Spectrum, Spectrum, Spectrum, spectrum)
DEFINE_PROPERTY_ACCESSOR(std::string, std::string, String, string)
DEFINE_PROPERTY_ACCESSOR(Properties::Data, Properties::Data, Data, data)

void Properties::setAnimatedTransform(const std::string &name, const AnimatedTransform *value, bool warnDuplicates) {
	if (hasProperty(name)) {
		AnimatedTransform **old = std::get_if<AnimatedTransform *>(&((*m_elements)[name].data));
		if (old)
			(*old)->decRef();
		if (warnDuplicates)
			SLog(EWarn, "Property \"%s\" was specified multiple times!", name.c_str());
	}
	(*m_elements)[name].data = (AnimatedTransform *) value;
	(*m_elements)[name].lastWrite = m_setCounter++;
	(*m_elements)[name].queried = false;
	value->incRef();
}

ref<const AnimatedTransform> Properties::getAnimatedTransform(const std::string &name) const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name);
	if (it == m_elements->end()) {
		SLog(m_mutateToRecord ? EWarn : EError, "Property \"%s\" missing", name.c_str());
		ref<const AnimatedTransform> defVal = new AnimatedTransform();
		MUTABLE_RECORD(setAnimatedTransform, defVal);
	}
	const AnimatedTransform * const * result1 = std::get_if<AnimatedTransform *>(&it->second.data);
	const Transform *result2 = std::get_if<Transform>(&it->second.data);

	if (!result1 && !result2)
		SLog(EError, "The property \"%s\" has the wrong type (expected <animation> or <transform>). The "
				"complete property record is :\n%s", name.c_str(), toString().c_str());
	it->second.queried = true;

	if (result1)
		return *result1;
	else
		return new AnimatedTransform(*result2);
}

ref<const AnimatedTransform> Properties::getAnimatedTransform(const std::string &name, const AnimatedTransform *defVal) const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name);
	if (it == m_elements->end())
		MUTABLE_RECORD(setAnimatedTransform, defVal);
	AnimatedTransform * const * result1 = std::get_if<AnimatedTransform *>(&it->second.data);
	const Transform *result2 = std::get_if<Transform>(&it->second.data);

	if (!result1 && !result2)
		SLog(EError, "The property \"%s\" has the wrong type (expected <animation> or <transform>). The "
				"complete property record is :\n%s", name.c_str(), toString().c_str());

	it->second.queried = true;

	if (result1)
		return *result1;
	else
		return new AnimatedTransform(*result2);
}

ref<const AnimatedTransform> Properties::getAnimatedTransform(const std::string &name, const Transform &defVal) const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name);
	if (it == m_elements->end()) {
		ref<const AnimatedTransform> defValAT = new AnimatedTransform(defVal);
		MUTABLE_RECORD(setAnimatedTransform, defValAT);
	}

	AnimatedTransform * const * result1 = std::get_if<AnimatedTransform *>(&it->second.data);
	const Transform *result2 = std::get_if<Transform>(&it->second.data);

	if (!result1 && !result2)
		SLog(EError, "The property \"%s\" has the wrong type (expected <animation> or <transform>). The "
				"complete property record is :\n%s", name.c_str(), toString().c_str());
	it->second.queried = true;

	if (result1)
		return *result1;
	else
		return new AnimatedTransform(*result2);
}

namespace {
	class TypeVisitor {
	public:
		Properties::EPropertyType operator()(const bool &) const              { return Properties::EBoolean; }
		Properties::EPropertyType operator()(const int64_t &) const           { return Properties::EInteger; }
		Properties::EPropertyType operator()(const Float &) const             { return Properties::EFloat; }
		Properties::EPropertyType operator()(const Point &) const             { return Properties::EPoint; }
		Properties::EPropertyType operator()(const Vector &) const            { return Properties::EVector; }
		Properties::EPropertyType operator()(const Transform &) const         { return Properties::ETransform; }
		Properties::EPropertyType operator()(const AnimatedTransform *) const { return Properties::EAnimatedTransform; }
		Properties::EPropertyType operator()(const Spectrum &) const          { return Properties::ESpectrum; }
		Properties::EPropertyType operator()(const std::string &) const       { return Properties::EString; }
		Properties::EPropertyType operator()(const Properties::Data &) const  { return Properties::EData; }
	};

	class EqualityVisitor {
	public:
		EqualityVisitor(const ElementData *ref) : ref(ref) { }

		bool operator()(const bool &v) const              { const bool *v2 = std::get_if<bool>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const int64_t &v) const           { const int64_t *v2 = std::get_if<int64_t>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const Float &v) const             { const Float *v2 = std::get_if<Float>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const Point &v) const             { const Point *v2 = std::get_if<Point>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const Vector &v) const            { const Vector *v2 = std::get_if<Vector>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const Transform &v) const         { const Transform *v2 = std::get_if<Transform>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const AnimatedTransform *v) const { AnimatedTransform * const *v2 = std::get_if<AnimatedTransform*>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const Spectrum &v) const          { const Spectrum *v2 = std::get_if<Spectrum>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const std::string &v) const       { const std::string *v2 = std::get_if<std::string>(ref); return v2 ? (v == *v2) : false; }
		bool operator()(const Properties::Data &v) const  { const Properties::Data *v2 = std::get_if<Properties::Data>(ref); return v2 ? (v == *v2) : false; }
	private:
		const ElementData *ref;
	};

	class StringVisitor {
	public:
		StringVisitor(std::ostringstream &oss, bool quote) : oss(oss), quote(quote) { }

		void operator()(const bool &v) const              { oss << (v ? "true" : "false"); }
		void operator()(const int64_t &v) const           { oss << v; }
		void operator()(const Float &v) const             { oss << v; }
		void operator()(const Point &v) const             { oss << v.toString(); }
		void operator()(const Vector &v) const            { oss << v.toString(); }
		void operator()(const Transform &v) const         { oss << v.toString(); }
		void operator()(const AnimatedTransform *v) const { oss << ((Object *) v)->toString(); }
		void operator()(const Spectrum &v) const          { oss << v.toString(); }
		void operator()(const std::string &v) const       { oss << (quote ? "\"" : "") << v << (quote ? "\"" : ""); }
		void operator()(const Properties::Data &v) const  { oss << v.ptr << " (size=" << v.size << ")"; }
	private:
		std::ostringstream &oss;
		bool quote;
	};
}

Properties::Properties()
: m_id("unnamed"), m_setCounter(0), m_mutateToRecord(false) {
	m_elements = new std::map<std::string, PropertyElement>();
}

Properties::Properties(const std::string &pluginName)
: m_pluginName(pluginName), m_id("unnamed"), m_setCounter(0), m_mutateToRecord(false) {
	m_elements = new std::map<std::string, PropertyElement>();
}

Properties::Properties(const Properties &props)
: m_pluginName(props.m_pluginName), m_id(props.m_id), m_setCounter(props.m_setCounter), m_mutateToRecord(props.m_mutateToRecord) {
	m_elements = new std::map<std::string, PropertyElement>(*props.m_elements);

	for (std::map<std::string, PropertyElement>::iterator it = m_elements->begin();
			it != m_elements->end(); ++it) {
		AnimatedTransform **trafo = std::get_if<AnimatedTransform *>(&(*it).second.data);
		if (trafo)
			(*trafo)->incRef();
	}
}

Properties::~Properties() {
	for (std::map<std::string, PropertyElement>::iterator it = m_elements->begin();
			it != m_elements->end(); ++it) {
		AnimatedTransform **trafo = std::get_if<AnimatedTransform *>(&(*it).second.data);
		if (trafo)
			(*trafo)->decRef();
	}

	delete m_elements;
}

void Properties::operator=(const Properties &props) {
	for (std::map<std::string, PropertyElement>::iterator it = m_elements->begin();
			it != m_elements->end(); ++it) {
		AnimatedTransform **trafo = std::get_if<AnimatedTransform *>(&(*it).second.data);
		if (trafo)
			(*trafo)->decRef();
	}

	m_pluginName = props.m_pluginName;
	m_id = props.m_id;
	*m_elements = *props.m_elements;

	for (std::map<std::string, PropertyElement>::iterator it = m_elements->begin();
			it != m_elements->end(); ++it) {
		AnimatedTransform **trafo = std::get_if<AnimatedTransform *>(&(*it).second.data);
		if (trafo)
			(*trafo)->incRef();
	}
}

bool Properties::hasProperty(const std::string &name) const {
	return m_elements->find(name) != m_elements->end();
}

bool Properties::removeProperty(const std::string &name) {
	std::map<std::string, PropertyElement>::iterator it = m_elements->find(name);
	if (it == m_elements->end())
		return false;
	AnimatedTransform **trafo = std::get_if<AnimatedTransform *>(&(*it).second.data);
	if (trafo)
		(*trafo)->decRef();
	m_elements->erase(it);
	return true;
}

std::vector<std::string> Properties::getUnqueried() const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->begin();
	std::vector<std::string> result;

	for (; it != m_elements->end(); ++it) {
		if (!(*it).second.queried)
			result.push_back((*it).first);
	}

	return result;
}

Properties::EPropertyType Properties::getType(const std::string &name) const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name);
	if (it == m_elements->end())
		SLog(EError, "Property \"%s\" has not been specified!", name.c_str());

	return std::visit(TypeVisitor(), it->second.data);
}

std::string Properties::getAsString(const std::string &name, const std::string &defVal) const {
	if (m_elements->find(name) == m_elements->end()) {
		if (m_mutateToRecord)
			SLog(EWarn, "Property \"%s\" not recorded because of untyped parameter query!", name.c_str());
		return defVal;
	}
	return getAsString(name);
}

std::string Properties::getAsString(const std::string &name) const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name);
	if (it == m_elements->end()) {
		if (m_mutateToRecord)
			SLog(EWarn, "Property \"%s\" not recorded because of untyped parameter query!", name.c_str());
		SLog(EError, "Property \"%s\" has not been specified!", name.c_str());
	}

	std::ostringstream oss;
	StringVisitor strVisitor(oss, false);
	std::visit(strVisitor, it->second.data);
	it->second.queried = true;

	return oss.str();
}

std::string Properties::toString() const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->begin();
	std::ostringstream oss;
	StringVisitor strVisitor(oss, true);

	oss << "Properties[" << endl
		<< "  pluginName = \"" << m_pluginName << "\"," << endl
		<< "  id = \"" << m_id << "\"," << endl
		<< "  elements = {" << endl;
	while (it != m_elements->end()) {
		oss << "    \"" << (*it).first << "\" -> ";
		const ElementData &data = (*it).second.data;
		std::visit(strVisitor, data);
		if (++it != m_elements->end())
			oss << ",";
		oss << endl;
	}
	oss << "  }" << endl
		<< "]" << endl;
	return oss.str();
}

void Properties::markQueried(const std::string &name) const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name);
	if (it == m_elements->end())
		return;
	it->second.queried = true;
}

bool Properties::wasQueried(const std::string &name) const {
	std::map<std::string, PropertyElement>::const_iterator it = m_elements->find(name);
	if (it == m_elements->end())
		SLog(EError, "Could not find parameter \"%s\"!", name.c_str());
	return it->second.queried;
}

void Properties::putPropertyNames(std::vector<std::string> &results) const {
	size_t numBefore = results.size();
	for (std::map<std::string, PropertyElement>::const_iterator it = m_elements->begin();
			it != m_elements->end(); ++it)
		results.push_back((*it).first);
	// add to the overall inefficiency of this class ;-)
	std::sort(results.begin() + numBefore, results.end()
		, [this](std::string const& a, std::string const& b) { return (*this->m_elements)[a].lastWrite < (*this->m_elements)[b].lastWrite; }
	);
}

void Properties::copyAttribute(const Properties &properties,
	const std::string &sourceName, const std::string &targetName) {
	std::map<std::string, PropertyElement>::const_iterator it = properties.m_elements->find(sourceName);
	if (it == properties.m_elements->end())
		SLog(EError, "copyAttribute(): Could not find parameter \"%s\"!", sourceName.c_str());
	m_elements->operator[](targetName) = it->second;
}

bool Properties::operator==(const Properties &p) const {
	if (m_pluginName != p.m_pluginName || m_id != p.m_id || m_elements->size() != p.m_elements->size())
		return false;

	std::map<std::string, PropertyElement>::const_iterator it = m_elements->begin();
	for (; it != m_elements->end(); ++it) {
		const PropertyElement &first = it->second;
		const PropertyElement &second = (*p.m_elements)[it->first];

		if (!std::visit(EqualityVisitor(&first.data), second.data))
			return false;
	}

	return true;
}

void Properties::merge(const Properties &p, const Properties *defaults, bool existingOnly) {
	std::map<std::string, PropertyElement>::const_iterator it = p.m_elements->begin();
	for (; it != p.m_elements->end(); ++it) {
		// not the default value
		if (defaults && std::visit(EqualityVisitor(&it->second.data), (*defaults->m_elements)[it->first].data))
			continue;
		std::pair<std::map<std::string, PropertyElement>::iterator, bool> iit;
		if (existingOnly) {
			iit.first = m_elements->find(it->first);
			iit.second = (iit.first == m_elements->end());
		}
		else
			iit = m_elements->insert(*it);
		// conflict
		if (!iit.second) {
			// same type
			if (iit.first->second.data.index() == it->second.data.index()) {
				// override
				iit.first->second = it->second;
			}
		}
	}
}

void Properties::copyQueriedFlags(const Properties& other) {
	std::map<std::string, PropertyElement>::const_iterator it = other.m_elements->begin();
	for (; it != other.m_elements->end(); ++it) {
		std::map<std::string, PropertyElement>::iterator iit = m_elements->find(it->first);
		if (iit != m_elements->end()) {
			iit->second.queried = it->second.queried;
		}
	}
}

ConfigurableObject::ConfigurableObject(Stream *stream, InstanceManager *manager)
 : SerializableObject(stream, manager) {
}

void ConfigurableObject::setParent(ConfigurableObject *parent) {
}

void ConfigurableObject::configure() {
}

void ConfigurableObject::serialize(Stream *stream, InstanceManager *manager) const {
	if (!getClass()->isSerializable())
		Log(EError, "Error: trying to serialize an instance of type '%s', which does "
			"not have full serialization support!", getClass()->getName().c_str());
}

void ConfigurableObject::addChild(const std::string &name, ConfigurableObject *child) {
	SLog(EError, "ConfigurableObject::addChild(\"%s\", %s) not implemented in \"%s\"",
		name.c_str(), child->toString().c_str(), toString().c_str());
}

void NetworkedObject::serialize(Stream *stream, InstanceManager *manager) const {
	ConfigurableObject::serialize(stream, manager);
}

void NetworkedObject::bindUsedResources(ParallelProcess *proc) const {
}

void NetworkedObject::wakeup(ConfigurableObject *,
	std::map<std::string, SerializableObject *> &) {
}

MTS_IMPLEMENT_CLASS(ConfigurableObject, true, SerializableObject)
MTS_IMPLEMENT_CLASS(NetworkedObject, true, ConfigurableObject)
MTS_NAMESPACE_END
