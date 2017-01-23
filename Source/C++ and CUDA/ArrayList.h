#include <vector>

template<class T> 
class ArrayList{

private:
	std::vector<T> list;

public:
	void add(T element);
	void add(int index, T element);
	T remove(int index);
	T removeByElement(T element);
	T removeAll(T element);
	bool contains(T element);

	void clear();

	T get(int index);
	int size();
		
};


template<class T> void ArrayList<T>::add(const T element){
	list.push_back(element);
}

template<class T> void ArrayList<T>::add(const int index, const T element){
	list.insert(list.begin() + index, element);
}

template<class T> T ArrayList<T>::get(const int index){
	return list.at(index);
}

template<class T> int ArrayList<T>::size(){
	return list.size();
}

template<class T> void ArrayList<T>::clear(){
	list.clear();
}
template<class T> bool ArrayList<T>::contains(const T element){
	for (int k = 0; k < this->size(); k++){
		if (this->get(k) == element)
			return true;
	}
	return false;
}

template<class T> T ArrayList<T>::remove(const int index){
	T auxiliar = this->get(index);
	list.erase(list.begin() + index);
	return auxiliar;
}
template<class T> T ArrayList<T>::removeByElement(const T element){
	T auxiliar;
	for (int k = 0; k < this->size(); k++){
		if (this->get(k) == element){
			auxiliar = this->remove(k);
			break;
		}
	}
	return auxiliar;
}

template<class T> T ArrayList<T>::removeAll(const T element){
	T auxiliar;
	for (int k = 0; k < this->size(); k++){
		if (this->get(k) == element){
			auxiliar = this->remove(k);
		}
	}
	return auxiliar;
}