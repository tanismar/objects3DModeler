// This is an automatically-generated file.
// It could get re-generated if the ALLOW_IDL_GENERATION flag is on.

#include <Point3D.h>

bool Point3D::read_x(yarp::os::idl::WireReader& reader) {
  if (!reader.readDouble(x)) {
    reader.fail();
    return false;
  }
  return true;
}
bool Point3D::nested_read_x(yarp::os::idl::WireReader& reader) {
  if (!reader.readDouble(x)) {
    reader.fail();
    return false;
  }
  return true;
}
bool Point3D::read_y(yarp::os::idl::WireReader& reader) {
  if (!reader.readDouble(y)) {
    reader.fail();
    return false;
  }
  return true;
}
bool Point3D::nested_read_y(yarp::os::idl::WireReader& reader) {
  if (!reader.readDouble(y)) {
    reader.fail();
    return false;
  }
  return true;
}
bool Point3D::read_z(yarp::os::idl::WireReader& reader) {
  if (!reader.readDouble(z)) {
    reader.fail();
    return false;
  }
  return true;
}
bool Point3D::nested_read_z(yarp::os::idl::WireReader& reader) {
  if (!reader.readDouble(z)) {
    reader.fail();
    return false;
  }
  return true;
}
bool Point3D::read(yarp::os::idl::WireReader& reader) {
  if (!read_x(reader)) return false;
  if (!read_y(reader)) return false;
  if (!read_z(reader)) return false;
  return !reader.isError();
}

bool Point3D::read(yarp::os::ConnectionReader& connection) {
  yarp::os::idl::WireReader reader(connection);
  if (!reader.readListHeader(3)) return false;
  return read(reader);
}

bool Point3D::write_x(yarp::os::idl::WireWriter& writer) {
  if (!writer.writeDouble(x)) return false;
  return true;
}
bool Point3D::nested_write_x(yarp::os::idl::WireWriter& writer) {
  if (!writer.writeDouble(x)) return false;
  return true;
}
bool Point3D::write_y(yarp::os::idl::WireWriter& writer) {
  if (!writer.writeDouble(y)) return false;
  return true;
}
bool Point3D::nested_write_y(yarp::os::idl::WireWriter& writer) {
  if (!writer.writeDouble(y)) return false;
  return true;
}
bool Point3D::write_z(yarp::os::idl::WireWriter& writer) {
  if (!writer.writeDouble(z)) return false;
  return true;
}
bool Point3D::nested_write_z(yarp::os::idl::WireWriter& writer) {
  if (!writer.writeDouble(z)) return false;
  return true;
}
bool Point3D::write(yarp::os::idl::WireWriter& writer) {
  if (!write_x(writer)) return false;
  if (!write_y(writer)) return false;
  if (!write_z(writer)) return false;
  return !writer.isError();
}

bool Point3D::write(yarp::os::ConnectionWriter& connection) {
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(3)) return false;
  return write(writer);
}
bool Point3D::Editor::write(yarp::os::ConnectionWriter& connection) {
  if (!isValid()) return false;
  yarp::os::idl::WireWriter writer(connection);
  if (!writer.writeListHeader(dirty_count+1)) return false;
  if (!writer.writeString("patch")) return false;
  if (is_dirty_x) {
    if (!writer.writeListHeader(3)) return false;
    if (!writer.writeString("set")) return false;
    if (!writer.writeString("x")) return false;
    if (!obj->nested_write_x(writer)) return false;
  }
  if (is_dirty_y) {
    if (!writer.writeListHeader(3)) return false;
    if (!writer.writeString("set")) return false;
    if (!writer.writeString("y")) return false;
    if (!obj->nested_write_y(writer)) return false;
  }
  if (is_dirty_z) {
    if (!writer.writeListHeader(3)) return false;
    if (!writer.writeString("set")) return false;
    if (!writer.writeString("z")) return false;
    if (!obj->nested_write_z(writer)) return false;
  }
  return !writer.isError();
}
bool Point3D::Editor::read(yarp::os::ConnectionReader& connection) {
  if (!isValid()) return false;
  yarp::os::idl::WireReader reader(connection);
  reader.expectAccept();
  if (!reader.readListHeader()) return false;
  int len = reader.getLength();
  if (len==0) {
    yarp::os::idl::WireWriter writer(reader);
    if (writer.isNull()) return true;
    if (!writer.writeListHeader(1)) return false;
    writer.writeString("send: 'help' or 'patch (param1 val1) (param2 val2)'");
    return true;
  }
  yarp::os::ConstString tag;
  if (!reader.readString(tag)) return false;
  if (tag=="help") {
    yarp::os::idl::WireWriter writer(reader);
    if (writer.isNull()) return true;
    if (!writer.writeListHeader(2)) return false;
    if (!writer.writeTag("many",1, 0)) return false;
    if (reader.getLength()>0) {
      yarp::os::ConstString field;
      if (!reader.readString(field)) return false;
      if (field=="x") {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeString("double x")) return false;
      }
      if (field=="y") {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeString("double y")) return false;
      }
      if (field=="z") {
        if (!writer.writeListHeader(1)) return false;
        if (!writer.writeString("double z")) return false;
      }
    }
    if (!writer.writeListHeader(4)) return false;
    writer.writeString("*** Available fields:");
    writer.writeString("x");
    writer.writeString("y");
    writer.writeString("z");
    return true;
  }
  bool nested = true;
  bool have_act = false;
  if (tag!="patch") {
    if ((len-1)%2 != 0) return false;
    len = 1 + ((len-1)/2);
    nested = false;
    have_act = true;
  }
  for (int i=1; i<len; i++) {
    if (nested && !reader.readListHeader(3)) return false;
    yarp::os::ConstString act;
    yarp::os::ConstString key;
    if (have_act) {
      act = tag;
    } else {
      if (!reader.readString(act)) return false;
    }
    if (!reader.readString(key)) return false;
    // inefficient code follows, bug paulfitz to improve it
    if (key == "x") {
      will_set_x();
      if (!obj->nested_read_x(reader)) return false;
      did_set_x();
    } else if (key == "y") {
      will_set_y();
      if (!obj->nested_read_y(reader)) return false;
      did_set_y();
    } else if (key == "z") {
      will_set_z();
      if (!obj->nested_read_z(reader)) return false;
      did_set_z();
    } else {
      // would be useful to have a fallback here
    }
  }
  reader.accept();
  yarp::os::idl::WireWriter writer(reader);
  if (writer.isNull()) return true;
  writer.writeListHeader(1);
  writer.writeVocab(VOCAB2('o','k'));
  return true;
}

yarp::os::ConstString Point3D::toString() {
  yarp::os::Bottle b;
  b.read(*this);
  return b.toString();
}
