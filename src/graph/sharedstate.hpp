#ifndef SHAREDSTATES_H
#define SHAREDSTATES_H

#include <atomic>
#include <sstream>

enum class Permission {NONE=0, READ, WRITE};

class Permissions {
public:
    Permissions( Permission self = Permission::WRITE, Permission others = Permission::READ, Permission external = Permission::NONE ) :
    self_(self), others_(others), external_(external) {}
    
    Permission self() const { return self_; }
    Permission others() const { return others_; }
    Permission external() const { return external_; }
    
    bool IsCompatible( const Permissions& p ) {
        return !(others_==Permission::NONE || p.others()==Permission::NONE || 
        (others_==Permission::READ && p.self()!=Permission::READ) || 
        (self_!=Permission::READ && p.others()==Permission::READ) );
    }
    
protected:
    Permission self_;
    Permission others_;
    Permission external_;
};

class IState {
friend class IProcessor;
public:
    IState( Permissions& permissions, std::string units="", std::string description="" ) : 
    permissions_(permissions), units_(units), description_(description) {}
    
    const Permissions& permissions() const { return permissions_; }
    
    virtual bool IsCompatible( const IState* other ) = 0;
    
    virtual void Share( IState* master ) = 0;
    virtual void Unshare() = 0;
    
    void SetMaster() { is_master_ = true; }
    bool IsMaster() { return is_master_; }
    
    std::string units() { return units_; }
    std::string description() { return description_; }
    
    void set_units(std::string value) { units_ = value; }
    void set_description(std::string value) { description_ = value; }
    
protected:
    
    virtual std::string get_string() const = 0;
    virtual bool set_string( std::string & value ) = 0;
    
protected:
    Permissions permissions_;
    std::string units_;
    std::string description_;
    
    bool is_master_ = false;
    
};

template <typename T>
class State : public IState {
public:
    State( T default_value, Permissions permissions, std::string units="", std::string description="" ) : 
    IState(permissions, units, description), default_(default_value), state_(default_value) {
        
        shared_state_ = &state_; // by default use our own store
    }
    
    virtual bool IsCompatible( const IState* other ) override {
        
        try {
            auto cast = dynamic_cast<const State<T>*>( other );
            if (cast) {
                return permissions_.IsCompatible( cast->permissions() );
            } else { return false; }
        } catch ( const std::bad_cast& e ) {
            return false;
        }
    }
    
    virtual void Share( IState* master ) override {
        
        if ( IsMaster() ) {
            throw std::runtime_error( "Internal error. Attempting to reshare master." );
        }
        
        auto cast = dynamic_cast<State<T>*>( master );
        if (cast) {
            this->shared_state_ = cast->shared_state();
        } else {
            throw std::runtime_error( "Internal error. Bad cast!!" );
        }
    }
    
    virtual void Unshare() override {
        
        this->shared_state_ = &this->state_;
    }
    
protected:
    std::atomic<T>* shared_state() { return shared_state_; }
    
    // only for external control
    virtual std::string get_string() const override {
        
        return std::to_string( shared_state_->load() );
    }
    virtual bool set_string( std::string & value )  override {
        
        std::stringstream ss(value);
        T result;
        if (ss >> result) {
            shared_state_->store(result);
            return true;
        }
        return false;
    } 
    
protected:
    T default_;
    std::atomic<T> state_;
    std::atomic<T>* shared_state_ = nullptr;

};

template <typename T>
class ReadableState : public State<T> {
public:
    ReadableState( T default_value, Permission peers = Permission::WRITE, Permission external = Permission::NONE ) : State<T>( default_value, Permissions( Permission::READ, peers, external ) ) {}
    ReadableState( T default_value, std::string units="", std::string description="", Permission peers = Permission::WRITE, Permission external = Permission::NONE ) : State<T>( default_value, Permissions( Permission::READ, peers, external ), units, description ) {}
    
    const T get() const {
        
        return this->shared_state_->load();
    }

};

template <typename T>
class WritableState : public State<T> {
public:
    WritableState( T default_value, Permission peers = Permission::READ, Permission external = Permission::NONE ) : State<T>( default_value, Permissions( Permission::WRITE, peers, external ) ) {}
    WritableState( T default_value, std::string units="", std::string description="", Permission peers = Permission::READ, Permission external = Permission::NONE ) : State<T>( default_value, Permissions( Permission::WRITE, peers, external ), units, description ) {}
    
    const T get() const {
        
        return this->shared_state_->load();
    }
    void set( T value ) {
        
        this->shared_state_->store(value);
    }
    T exchange( T value ) {
        
        return this->shared_state_->exchange(value);
    }
};

#endif
