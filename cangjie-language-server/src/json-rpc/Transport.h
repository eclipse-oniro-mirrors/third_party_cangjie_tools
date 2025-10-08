// Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
// This source file is part of the Cangjie project, licensed under Apache-2.0
// with Runtime Library Exception.
//
// See https://cangjie-lang.cn/pages/LICENSE for license information.

// The Cangjie API is in Beta. For details on its capabilities and limitations, please refer to the README file.

#ifndef LSPSERVER_TRANSPORT_H
#define LSPSERVER_TRANSPORT_H

#include <iostream>
#include <mutex>
#include "Protocol.h"

namespace ark {
enum class LSPRet {
    SUCCESS = 0,
    NORMAL_EXIT,
    ABNORMAL_EXIT,
    ERR_JSON,
    ERR_IO
};

enum class ValueOrErrorCheck {
    ERR,
    VALUE
};

class ValueOrError {
public:
    ValueOrErrorCheck type;
    MessageErrorDetail errorInfo{};
    nlohmann::json jsonValue{};

    ValueOrError() : type(ValueOrErrorCheck::ERR), errorInfo() {}

    explicit ValueOrError(ValueOrErrorCheck type) : type(type) {}

    ValueOrError(ValueOrErrorCheck type, const MessageErrorDetail &errorInfo)
        : type(type),
          errorInfo(errorInfo) {}

    ValueOrError(ValueOrErrorCheck type, const nlohmann::json &jsonValue)
        : type(type),
          jsonValue(jsonValue) {}
          
    ~ValueOrError() {}
};

template <class TransportType_t>
class ITransportRegistrar {
public:
    virtual TransportType_t *CreateTransport() = 0;

    ITransportRegistrar(const ITransportRegistrar &) = delete;
    const ITransportRegistrar &operator=(const ITransportRegistrar &) = delete;
    virtual ~ITransportRegistrar() {}

protected:
    ITransportRegistrar() {}
};

template <class TransportType_t>
class TransportFactory {
public:
    static TransportFactory<TransportType_t> &Instance()
    {
        static TransportFactory<TransportType_t> instance {};
        return instance;
    }

    void RegisterTransport(ITransportRegistrar<TransportType_t> *registrar, const std::string &name)
    {
        m_TransportRegistry[name] = registrar;
    }

    TransportType_t *GetTransport(const std::string &name)
    {
        if (m_TransportRegistry.find(name) != m_TransportRegistry.end()) {
            return m_TransportRegistry[name]->CreateTransport();
        }

        return nullptr;
    }

    TransportFactory(const TransportFactory &) = delete;
    const TransportFactory &operator=(const TransportFactory &) = delete;

private:
    TransportFactory() {}
    ~TransportFactory() {}

    // key:name, value:transport type
    std::map<std::string, ITransportRegistrar<TransportType_t> *> m_TransportRegistry{};
};

template <class TransportType_t, class TransportImpl_t>
class TransportRegistrar : public ITransportRegistrar<TransportType_t> {
public:
    explicit TransportRegistrar(const std::string& name)
    {
        this->name = name;
    }

    void Regist()
    {
        TransportFactory<TransportType_t>::Instance().RegisterTransport(this, name);
    }

    ~TransportRegistrar() {}

    TransportType_t *CreateTransport() override
    {
        return &TransportImpl_t::Instance();
    }

private:
    std::string name {};
};

class Transport {
public:
    virtual void SetIO(std::FILE *in, std::FILE *out) = 0;

    virtual void Notify(std::string, ValueOrError) = 0;

    virtual void Reply(nlohmann::json, ValueOrError) = 0;

    class MessageHandler {
    public:
        virtual ~MessageHandler() = default;

        virtual LSPRet OnNotify(std::string, nlohmann::json) = 0;

        virtual LSPRet OnCall(std::string, nlohmann::json, nlohmann::json) = 0;

        virtual LSPRet OnReply(nlohmann::json, ValueOrError) = 0;
    };

    virtual LSPRet Loop(MessageHandler &) = 0;

    virtual ~Transport() {}

    std::mutex transpWriter{};
};
} // namespace ark

#endif // LSPSERVER_TRANSPORT_H
