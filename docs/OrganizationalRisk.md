### Organizational Risks

Managing a business's risk is best left to those with experience. Maplang can be used to reduce certain types of risks:

**Problem** Requirements change abruptly.

**Solution** A single piece of code can be updated or created to meet the new requirement, tested using real-world data recorded in production, then deployed without downtime.

-----
**Problem** Cloud provider experiences a catastrophic failure (plane crashes into a data center, company goes out of business, large-scale DDoS)
**Solution** Maplang is built to be multi-cloud, and works well on-prem too.
* How is it multi-cloud?
  * A [Graph Manager](GraphManager.md) orchestrates Maplang code. It detects failures and overloaded servers, and moves code to an optimal location.
  * It's free and open source. You can run Maplang on any server or cloud provider.
* What about on-prem?
  * Servers can be registered with the Graph Manager either at startup, or dynamically via the [API](GraphManagerAPI.md).
  * Provisioning VMs or bare metal server is out of scope, and needs to be handled by your IT department.


* Vendor experiences a catastrophic failure (plane crashes into a data center, 500-year flood wipes out servers, goes out of business)
* Vendor lock-in

Employees
* A key engineer is out of action - (vacation, hospitalization, quits, etc.)
  * Code is broken into small pieces out of necessity. Inputs and outputs are well-defined, so components can be re-created if necessary, and plugged into an existing system.
