part of webcl;

Uint8List allocateExternalList(int size) native "AllocateExternalList";

abstract class TypedArray<T extends List<int>> extends ByteArrayViewable {
  Uint8List _list8;
  T _view;
  int _bytesPerElement;
  TypedArray(int size, this._bytesPerElement){
    _list8 = allocateExternalList(size * _bytesPerElement);
  }
  int bytesPerElement() => _bytesPerElement;
  int lengthInBytes() => _list8.lengthInBytes() ~/ _bytesPerElement;
  ByteArray asByteArray([int start, int length]) => _list8.asByteArray();
  
  operator[]= (int index, val) => _view[index] = val;
  operator[] (int index) => _view[index];
}

class Uint32Array extends TypedArray {
  
  Uint32Array(int size) : super(size, 4) {
    _view = new Int32List.view(this.asByteArray(), 0, size);
  }

}
