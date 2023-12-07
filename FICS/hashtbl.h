#ifndef __FICSHashTable_h
#define __FICSHashTable_h
#ifdef __GNUC__
# pragma interface
#endif
//-----------------------------------------------------------------------------
// FICSHashTable.h
//
//	A general-purpose C++ hash table object.
//
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// $Id: FICSHashTable.h,v 1.0 1995/04/25 21:50:44 mabshier Exp $
// $Log: FICSHashTable.h,v $
// $Renamed file to HASHTBL.H to accomodate MSDOS file convention rename for Unix
// Initial revision
// 
//-----------------------------------------------------------------------------

#include <FICSContainer.h>
#include <FICSHash.h>

class FICSHashAdaptor;

class FICSHashTable : public FICSContainer
    {
public:
	unsigned long const DEFAULT_CAPACITY = 47;
	unsigned long const LOAD_FACTOR_SCALE = 16;
	unsigned long const LOAD_FACTOR_SHIFT = 4;
	unsigned long const LOAD_FACTOR_DEFAULT = 16;

private:
	FICSHashAdaptor* adaptor;
	unsigned long	max_load_factor;	// Scaled * 16.
	unsigned long	table_count;		// Number of entries.
	unsigned long	table_size;		// Number of slots.
	void**		table;
	void expand( unsigned long new_capacity );

public:
	FICSHashTable(
		FICSHashAdaptor&,
		unsigned long capacity = DEFAULT_CAPACITY,
		unsigned long max_load = LOAD_FACTOR_DEFAULT );
virtual	~FICSHashTable();
virtual	size_t vcount() const;
	size_t count() const	{ return table_count; }
	size_t capacity() const	{ return table_size; }
virtual	void empty();
virtual	bool findObj( void const* keyPtr, void const*& infoPtr ) const;
virtual	bool storeObj( void const* keyPtr, void const* infoPtr,
		FICSContainer::Unique u=FICSContainer::UNIQUE );
virtual	bool removeObj( void const* keyPtr );	// Removes 1st entry with key.
virtual	int  removeAllObj(void const* keyPtr);	// Removes all, returns count.

// These return a pointer to the hash_rec, or 0.
virtual	void const* findObj( void const* keyPtr ) const;
virtual	void const* findOrStoreObj( void const* keyPtr, void const* infoPtr );

friend class FICSHashTableIterator;
	FICSHashTableIterator* iterator() const;
virtual	FICSIterator* newIterator() const;

private: // Illegal
	FICSHashTable( FICSHashTable const& ) {}	// No copy constructor.
	void operator=(FICSHashTable const&) {}// No assignment.

protected:
	FICSHashValue getHash( void const* hash_rec ) const;
	FICSHashValue hash( void const* key ) const;
	void  setLink( void* hash_rec, void const* hash_link ) const;
	void const* getLink( void const* hash_rec ) const;
	bool  equal(void const* hash_rec,FICSHashValue,void const* key) const;
	void  deleteHashRec( void* );
	void* newHashRec( FICSHashValue hash_val, void const* key_ptr,
			void const* info, void const* link );
	void const* getInfo( void const* hash_rec ) const;
	void const* getKey( void const* hash_rec ) const;
	void fakeEmpty() { table_count = 0; }
    };


class FICSHashTableIterator : public FICSIterator
    {
private:
	FICSHashTable const& table;
	size_t next_slot;
	void const* curr_ptr;
public:
	void reset()	{ next_slot = 0; curr_ptr = 0; }
	FICSHashTableIterator( FICSHashTable const& x ):table(x) { reset(); }
virtual	~FICSHashTableIterator();
virtual	bool next( void*& );
    };

#endif // __FICSHashTable_h
